#pragma once

#include "icon.h"

extern const wchar_t * appNameW();


class NotifyIcon
{
    NotifyIcon(const NotifyIcon &) = delete;
    NotifyIcon & operator=(const NotifyIcon &) = delete;
public:
    NotifyIcon(int iconResId, HWND hWnd = 0);
    ~NotifyIcon();

    void setMenu(HMENU menu)
    {
        hMenu_ = menu;
    }

private:
    static constexpr int  StatusCallbackMessage = WM_APP + 1;
    int  TaskbarRestartMessage = 0;

    static ATOM registerClass();

    friend LRESULT WINAPI miniWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

    void setIcon(Icon & icon, bool isNew = false);

    Icon icon_;
    HWND hWnd_;
    HWND hMiniWnd_;
    HMENU hMenu_;
};

