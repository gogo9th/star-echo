
#include "framework.h"

#include "control.h"
#include "MainWindow.h"
#include "NotifyIcon.h"

#include "common/scopedResource.h"
#include "common/registry.h"
#include "common/settings.h"

#include "../apo/Q2APO.h"

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

static const wchar_t setOperaHall[] = L"ch,10,10";
static const wchar_t setLiveCafe[] = L"ch,13,10";

enum
{
    NI_OperaHall = WM_USER + 10,
    NI_LiveCafe,
    NI_Disable,
    NI_Enable,
    NI_Setup,
};

std::unique_ptr<MainWindow> mw_;
static INT_PTR WINAPI dlgProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case NI_OperaHall:
            break;
        case NI_LiveCafe:
            break;
        case NI_Disable:
            break;
        case NI_Setup:
            break;
    }

    return mw_->dlgProc(hWnd, uMsg, wParam, lParam);
}

static bool isUserInAdminGroup();


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

    try
    {
        const bool isAdmin = isUserInAdminGroup();
        bool showConfig = isAdmin;
        //if (wcscmp(lpCmdLine, L"setup") == 0) 
        {
            if (isAdmin)
            {
                Settings::setupAppKey();
            }

            //showConfig = true;
        }

        HMenu hMenu(CreatePopupMenu());

        if (Registry::keyExists(Settings::appPath))
        {
            InsertMenuW(hMenu, NI_OperaHall, MF_BYCOMMAND, NI_OperaHall, L"Opera Hall");
            InsertMenuW(hMenu, NI_LiveCafe, MF_BYCOMMAND, NI_LiveCafe, L"Live cafe");
            InsertMenuW(hMenu, -1, MF_SEPARATOR, 0, nullptr);
            InsertMenuW(hMenu, NI_Disable, MF_BYCOMMAND, NI_Disable, L"Disable");
            InsertMenuW(hMenu, -1, MF_SEPARATOR, 0, nullptr);

            auto current = Settings::current();
            if (!Settings::isEnabled(current))
            {
                CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_Disable, MF_BYCOMMAND);
            }
            else
            {
                if (current == setOperaHall)
                {
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_OperaHall, MF_BYCOMMAND);
                }
                else if (current == setLiveCafe)
                {
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_LiveCafe, MF_BYCOMMAND);
                }
                else
                {
                    Settings::set(setOperaHall);
                    CheckMenuRadioItem(hMenu, NI_OperaHall, NI_Disable, NI_OperaHall, MF_BYCOMMAND);
                }
            }
        }

        InsertMenuW(hMenu, NI_Setup, MF_BYCOMMAND, NI_Setup, L"Setup");
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
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

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

static bool isUserInAdminGroup()
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
