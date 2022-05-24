#pragma once

#include "framework.h"
#include "icon.h"

class MainWindow
{
    MainWindow(const MainWindow &) = delete;
    MainWindow operator=(const MainWindow &) = delete;

public:
    MainWindow(DLGPROC dlgProc);
    ~MainWindow();

    void create();

    void show(bool visible = true)
    {
        ShowWindow(hWnd_, visible ? SW_SHOW : SW_HIDE);
    }

private:
    friend INT_PTR WINAPI dlgProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    Icon    icon_;
    HWND    hWnd_ = 0;
    DLGPROC dlgProc_;
};

