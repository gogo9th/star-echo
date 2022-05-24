
#include "framework.h"

#include "control.h"
#include "MainWindow.h"


#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


static const char appTitle_[]{ "Ultraverse" };
static HINSTANCE hInstance_;

HINSTANCE hInstance()
{
    return hInstance_;
}

const wchar_t * appNameW()
{
    return L"Ultraverse";
}

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

    bool showConfig = false;
    //if (wcscmp(lpCmdLine, L"setup") == 0) 
    {
        showConfig = true;
    }

    try
    {
        mw_ = std::make_unique<MainWindow>(dlgProc_);
        mw_->create();
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
        return MessageBoxA(0, ("Error : "s + e.what()).c_str(), appTitle_, MB_OK | MB_ICONERROR);
    }
    catch (const std::exception & e)
    {
        return MessageBoxA(0, ("Exception : "s + e.what()).c_str(), appTitle_, MB_OK | MB_ICONERROR);
    }

    CoUninitialize();

    return -1;
}
