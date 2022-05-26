
#include "framework.h"

#include <map>

#include <shellapi.h>
#include "NotifyIcon.h"


//

static std::map<HWND, NotifyIcon *> miniWindows_;
static INT_PTR WINAPI miniWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto iw = miniWindows_.find(hWnd);
    return iw != miniWindows_.end() ? iw->second->WndProc(uMsg, wParam, lParam) : DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

NotifyIcon::NotifyIcon(int iconResId, HWND hWnd)
    : icon_(iconResId)
    , hWnd_(hWnd)
{
    // initialize once
    static ATOM atom = registerClass();
    if (atom == NULL) throw Error::gle("Failed to register miniwindow class");

    hMiniWnd_ = CreateWindowExW(0, MAKEINTATOM(atom), L"", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, 0,
                                CW_USEDEFAULT, 0,
                                HWND_MESSAGE,
                                nullptr,
                                hInstance(),
                                nullptr);
    if (hMiniWnd_ == NULL) throw Error::gle("Failed to create miniwindow");
    miniWindows_[hMiniWnd_] = this;
    setIcon(icon_, true);
}

NotifyIcon::~NotifyIcon()
{
    NOTIFYICONDATAW niData{};
    niData.cbSize = sizeof(niData);
    niData.uVersion = NOTIFYICON_VERSION;
    niData.hWnd = hMiniWnd_;
    niData.uID = 0;

    Shell_NotifyIconW(NIM_DELETE, &niData);
}

ATOM NotifyIcon::registerClass()
{
    WNDCLASSEXW mini = { 0 };
    mini.cbSize = sizeof(mini);

    mini.lpfnWndProc = miniWndProc;
    mini.hInstance = hInstance();
    mini.lpszClassName = L"notifyIconMiniWindow";
    return RegisterClassExW(&mini);
}

void NotifyIcon::setIcon(Icon & icon, bool isNew /*= false*/)
{
    NOTIFYICONDATAW niData{};
    niData.cbSize = sizeof(niData);
    niData.uVersion = NOTIFYICON_VERSION;
    niData.hWnd = hMiniWnd_;
    niData.uID = 0;
    wcscpy_s(niData.szTip, appNameW());
    niData.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    niData.uCallbackMessage = StatusCallbackMessage;
    niData.hIcon = icon;

    Shell_NotifyIconW(isNew ? NIM_ADD : NIM_MODIFY, &niData);
}

LRESULT NotifyIcon::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case StatusCallbackMessage:
        {
            switch (lParam)
            {
                //case WM_LBUTTONDBLCLK:
                //    ShowWindow(hWnd, SW_RESTORE);
                //    break;
                case WM_RBUTTONDOWN:
                case WM_CONTEXTMENU:
                {
                    SetForegroundWindow(hMiniWnd_);

                    POINT pt;
                    GetCursorPos(&pt);
                    auto r = TrackPopupMenu(hMenu_, TPM_LEFTBUTTON | TPM_RETURNCMD,
                                            pt.x, pt.y, 0, hMiniWnd_, NULL);
                    if (r != 0)
                    {
                        PostMessageW(hWnd_, r, 0, 0);
                    }

                    break;
                }
            }
            break;
        }

        default:
            return DefWindowProc(hMiniWnd_, uMsg, wParam, lParam);
            break;
    }
    return 0;
}
