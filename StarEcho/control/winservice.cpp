
#include "framework.h"

#include <chrono>
#include <thread>

#include "winservice.h"
#include "common/error.h"



using namespace std::chrono_literals;

SCM::SCM()
{
    scm_ = OpenSCManagerW(0, 0, SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_CONNECT);
    if (scm_ == 0)
    {
        throw WError::gle(L"Failed to open service manager");
    }
}

SCM::~SCM()
{
    CloseServiceHandle(scm_);
}

std::shared_ptr<SCM::Service> SCM::service(const wchar_t * name)
{
    return std::shared_ptr<SCM::Service>(new Service(std::move(shared_from_this()), name));
}

SCM::Service::Service(std::shared_ptr<SCM> && cm, const wchar_t * name)
    : cm_(cm)
{
    service_ = OpenServiceW(cm_->scm_, name, SERVICE_QUERY_STATUS | SERVICE_ENUMERATE_DEPENDENTS | SERVICE_INTERROGATE | SERVICE_START | SERVICE_STOP);
    if (service_ == 0)
        throw WError::gle(L"Failed to open service");
}

SCM::Service::~Service()
{
    CloseServiceHandle(service_);
}

struct SCM::Service::Status SCM::Service::status() const
{
    DWORD bn;
    SERVICE_STATUS_PROCESS ssStatus;
    if (QueryServiceStatusEx(service_, SC_STATUS_PROCESS_INFO, (BYTE *)&ssStatus, sizeof(ssStatus), &bn) == 0)
        throw WError::gle(L"Failed to query service");

    return { ssStatus.dwCurrentState, ssStatus.dwProcessId };
}

bool SCM::Service::start(unsigned timeout)
{
    return operation(Operation::Start, timeout);
}

bool SCM::Service::stop(unsigned timeout)
{
    return operation(Operation::Stop, timeout);
}

std::vector<std::shared_ptr<SCM::Service>> SCM::Service::dependencies()
{
    DWORD bNeeded = 0, nServices = 0;
    std::unique_ptr<byte> d;

    while (!EnumDependentServicesW(service_, SERVICE_STATE_ALL, (ENUM_SERVICE_STATUSW *)d.get(), bNeeded, &bNeeded, &nServices))
    {
        auto e = GetLastError();
        if (e == ERROR_MORE_DATA)
            d.reset(new byte[bNeeded]);
        else if (e != ERROR_SUCCESS)
            throw WError::gle(L"Failed to get dependent services");
    }

    std::vector<std::shared_ptr<SCM::Service>> r;
    for (DWORD i = 0; i < nServices; i++)
    {
        auto & ss = *((ENUM_SERVICE_STATUSW *)d.get() + i);
        r.push_back(cm_->service(ss.lpServiceName));
    }
    return r;
}

bool SCM::Service::operation(Operation operation, unsigned timeout)
{
    DWORD bn;
    SERVICE_STATUS_PROCESS ssStatus;

    do
    {
        if (QueryServiceStatusEx(service_, SC_STATUS_PROCESS_INFO, (BYTE *)&ssStatus, sizeof(ssStatus), &bn) == 0)
            throw WError::gle(L"Failed to query service");
        switch (ssStatus.dwCurrentState)
        {
            case SERVICE_RUNNING:
            {
                SERVICE_STATUS status;
                if (operation == Operation::Stop)
                {
                    if (!ControlService(service_, SERVICE_CONTROL_STOP, &status))
                        throw WError::gle(L"Failed to stop service");
                }
                else if (operation == Operation::Start)
                {
                    return true;
                }
                break;
            }

            case SERVICE_STOPPED:
            case SERVICE_PAUSED:
                if (operation == Operation::Start)
                {
                    if (!StartServiceW(service_, 0, 0))
                        throw WError::gle(L"Failed to start service");
                }
                else if (operation == Operation::Stop)
                {
                    return true;
                }
                break;

            case SERVICE_STOP_PENDING:
            case SERVICE_START_PENDING:
            case SERVICE_PAUSE_PENDING:
            case SERVICE_CONTINUE_PENDING:
                if (timeout == 0)
                {
                    error_ = ERROR_TIMEOUT;
                    return false;
                }
                std::this_thread::sleep_for(500ms);
                --timeout;
                break;

            default:
                assert(0);
                break;
        }

        std::this_thread::sleep_for(500ms);

    } while (timeout > 0);

    return false;
}
