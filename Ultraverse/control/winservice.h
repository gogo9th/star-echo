#pragma once

#include <winsvc.h>

#include <memory>

class SCM : public std::enable_shared_from_this<SCM>
{
    SCM(const SCM &) = delete;
    SCM operator=(const SCM &) = delete;
public:
    class Service
    {
        Service(std::shared_ptr<SCM> && cm, const wchar_t * name);
    public:
        ~Service();

        struct status { unsigned state; unsigned pid; } status() const;
        bool start(unsigned timeout = 10 /*seconds*/);
        bool stop(unsigned timeout = 10 /*seconds*/);

        auto error() const
        {
            return error_;
        }

    private:
        enum class Operation
        {
            Start,
            Stop,
        };
        bool operation(Operation operation, unsigned timeout);

        friend class SCM;

        std::shared_ptr<SCM> cm_;
        SC_HANDLE service_;

        unsigned error_ = 0;
    };

    SCM();
    ~SCM();

    std::shared_ptr<SCM::Service> service(const wchar_t * name);

private:

    SC_HANDLE scm_;
};

