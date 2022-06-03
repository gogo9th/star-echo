
#include "framework.h"

#include <Shellapi.h>

#include "control.h"
#include "MainWindow.h"
#include "NotifyIcon.h"
#include "winservice.h"

#include "common/scopedResource.h"
#include "common/registry.h"
#include "common/settings.h"

#include "../apo/Q2APO.h"
#include <control/mmDevices.h>

//#include <Shlobj.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


static HINSTANCE hInstance_;

HINSTANCE hInstance()
{
    return hInstance_;
}

const wchar_t * appNameW()
{
    return L"Ultraverse";
}

const char * appName()
{
    return "Ultraverse";
}

//

static const std::wstring selfExeFilepath()
{
    wchar_t selfName[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, selfName, (int)std::size(selfName));
    return selfName;
}

//

static const wchar_t operaHallMode[] = L"ch,10,10";
static const wchar_t liveCafeMode[] = L"ch,13,10";

enum
{
    NI_OperaHall = WM_USER + 10,
    NI_LiveCafe,
    NI_Disable,
    NI_Enable,
    NI_Setup,
    NI_Autostart,
};

std::unique_ptr<MainWindow> mw_;
static INT_PTR WINAPI dlgProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return mw_->dlgProc(hWnd, uMsg, wParam, lParam);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR    lpCmdLine,
                      _In_ int       nCmdShow)
{
    using namespace std::literals::string_literals;

    hInstance_ = hInstance;
    UNREFERENCED_PARAMETER(hPrevInstance);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    const bool isAdmin = isUserInAdminGroup();

    try
    {
        if (isAdmin)
        {
            if (wcscmp(lpCmdLine, L"installDefault") == 0)
            {
                auto defaultDev = getDefaultMMDevice(eRender);
                auto devInfo = std::make_shared<MMDeviceInfo>(defaultDev);
                if (!devInfo->apoMfxInstalled)
                {
                    devInfo->installApo();
                    devInfo->enableEnhancements();
                    restartAudioService();
                }
                return 0;
            }
            else if (wcscmp(lpCmdLine, L"uninstall") == 0)
            {
                auto devices = getPlaybackDevices();
                bool wasRemoved = false;
                for (auto & device : devices)
                {
                    if (device->apoMfxInstalled)
                    {
                        device->removeApo();
                        wasRemoved = true;
                    }
                }
                if (wasRemoved)
                {
                    restartAudioService();
                }
                return 0;
            }
        }


        bool showConfig = false;
        auto wPreviousInstance = FindWindowW(nullptr, appNameW());

        if (wcscmp(lpCmdLine, L"setup") == 0) 
        {
            // when launched with setup option, close previous instance
            if (wPreviousInstance != NULL)
            {
                //auto threadId = ::GetWindowThreadProcessId(wPreviousInstance, nullptr);
                //PostThreadMessageW(threadId, WM_QUIT, 0, 0);
                PostMessageW(wPreviousInstance, WM_QUIT, 0, 0);
            }

            if (isAdmin)
            {
                Settings::setupAppKey();
            }

            showConfig = true;
        }
        else if (wPreviousInstance != NULL)
        {
            // do not launch more instances without setup option
            return 0;
        }

        HMenu hMenu(CreatePopupMenu());

        if (Registry::keyExists(Settings::appPath))
        {
            InsertMenuW(hMenu, NI_OperaHall, MF_BYCOMMAND, NI_OperaHall, L"Opera Hall");
            InsertMenuW(hMenu, NI_LiveCafe, MF_BYCOMMAND, NI_LiveCafe, L"Live Cafe");
            InsertMenuW(hMenu, -1, MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hMenu, NI_Disable, MF_BYCOMMAND, NI_Disable, L"Disable");
            InsertMenuW(hMenu, -1, MF_SEPARATOR, 0, nullptr);

            auto current = Settings::currentEffect();
            if (!Settings::isEffectEnabled(current))
            {
                CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_Disable, MF_BYCOMMAND);
            }
            else
            {
                if (current == operaHallMode)
                {
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_OperaHall, MF_BYCOMMAND);
                }
                else if (current == liveCafeMode)
                {
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_LiveCafe, MF_BYCOMMAND);
                }
                else
                {
                    Settings::setEffect(operaHallMode);
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_OperaHall, MF_BYCOMMAND);
                }
            }
        }

        InsertMenuW(hMenu, NI_Setup, MF_BYCOMMAND, NI_Setup, L"Setup");
        if (!isAdmin)
        {
            SHSTOCKICONINFO stockIconInfo;
            stockIconInfo.cbSize = sizeof(stockIconInfo);
            if (SHGetStockIconInfo(SHSTOCKICONID::SIID_SHIELD, SHGSI_ICON | SHGSI_SMALLICON, &stockIconInfo) == S_OK)
            {
                Icon icon(stockIconInfo.hIcon, true);
                ICONINFO iconInfo;
                GetIconInfo(icon.small(), &iconInfo);

                MENUITEMINFOW mi;
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_BITMAP;
                //mi.hbmpItem = iconInfo.hbmColor;
                mi.hbmpItem = (HBITMAP)CopyImage(iconInfo.hbmColor, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
                SetMenuItemInfoW(hMenu, NI_Setup, FALSE, &mi);

                //SetMenuItemBitmaps()
            }
        }

        InsertMenuW(hMenu, NI_Autostart, MF_BYCOMMAND, NI_Autostart, L"Launch on Startup");
        CheckMenuItem(hMenu, NI_Autostart, MF_BYCOMMAND | (Settings::isAutostart(appNameW(), selfExeFilepath()) ? MF_CHECKED : MF_UNCHECKED));

        InsertMenuW(hMenu, WM_QUIT, MF_BYCOMMAND, WM_QUIT, L"Exit");

        mw_ = std::make_unique<MainWindow>(dlgProc_);
        mw_->create();

        NotifyIcon ni(IDC_MYICON, *mw_);
        ni.setMenu(hMenu);

        if (showConfig)
        {
            mw_->show();
        }

        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0))
        {
            switch (msg.message)
            {
                case NI_OperaHall:
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_OperaHall, MF_BYCOMMAND);
                    Settings::setEffect(operaHallMode);
                    break;
                case NI_LiveCafe:
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_LiveCafe, MF_BYCOMMAND);
                    Settings::setEffect(liveCafeMode);
                    break;
                case NI_Disable:
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_Disable, MF_BYCOMMAND);
                    Settings::setEffect(Settings::effectDisabledString());
                    break;
                case NI_Setup:
                    if (isAdmin)
                    {
                        mw_->show();
                    }
                    else
                    {
                        ShellExecuteW(NULL, L"runas", selfExeFilepath().data(), L"setup", NULL, SW_SHOWDEFAULT);
                    }
                    break;
                case NI_Autostart:
                {
                    auto current = GetMenuState(hMenu, NI_Autostart, MF_BYCOMMAND | MF_CHECKED);
                    current = !current;
                    if (current)
                    {
                        Settings::setAutostart(appNameW(), selfExeFilepath());
                    }
                    else
                    {
                        Settings::removeAutostart(appNameW());
                    }
                    CheckMenuItem(hMenu, NI_Autostart, MF_BYCOMMAND | (current ? MF_CHECKED : MF_UNCHECKED));
                    break;
                }

                default:
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
            }
        }

        Settings::setEffect(Settings::effectDisabledString());

        return 0;
    }
    catch (const Error & e)
    {
        return MessageBoxA(0, ("Error : "s + e.what()).c_str(), appName(), MB_OK | MB_ICONERROR);
    }
    catch (const WError & e)
    {
        return MessageBoxW(0, (L"Error : "s + e.what()).c_str(), appNameW(), MB_OK | MB_ICONERROR);
    }
    catch (const std::exception & e)
    {
        return MessageBoxA(0, ("Exception : "s + e.what()).c_str(), appName(), MB_OK | MB_ICONERROR);
    }

    CoUninitialize();

    return -1;
}

bool isUserInAdminGroup()
{
    BOOL r;

    PSid adminGroup;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    r = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &adminGroup);
    if (r)
    {
        if (!CheckTokenMembership(NULL, adminGroup, &r))
        {
            r = FALSE;
        }
    }

    return r;
}

int restartAudioService()
{
    auto scm = std::make_shared<SCM>();
    auto service = scm->service(L"audiosrv");
    auto status = service->status();
    if (status.state == SERVICE_RUNNING)
    {
        if (!service->stop()
            || service->status().state != SERVICE_STOPPED)
        {
            return -1;
        }
    }
    if (!service->start()
        || service->status().state != SERVICE_RUNNING)
    {
        return -1;
    }

    return 0;
}
