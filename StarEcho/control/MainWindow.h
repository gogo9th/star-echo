#pragma once

#include "framework.h"
#include "icon.h"
#include "mmDeviceInfo.h"

class MainWindow
{
    MainWindow(const MainWindow &) = delete;
    MainWindow operator=(const MainWindow &) = delete;

public:
    MainWindow(DLGPROC dlgProc);
    ~MainWindow();

    operator HWND() const
    {
        return hWnd_;
    }

    void create();

    void show()
    {
        ShowWindow(hWnd_, SW_SHOW);
        SetForegroundWindow(hWnd_);
    }

    void hide()
    {
        ShowWindow(hWnd_, SW_HIDE);
    }

private:
    friend INT_PTR WINAPI dlgProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void onButtonClicked(int id);

    void updateDeviceFromUi(int index);
    void updateDeviceInfos();
    void updateDeviceInfoStatus();

    Icon    icon_;
    HWND    hWnd_ = 0;
    DLGPROC dlgProc_;

    HWND    deviceList_;
    HWND    btnApply_;

    std::wstring                                defaultDevilceGuid_;
    std::vector<std::shared_ptr<MMDeviceInfo>>  deviceInfos_;
};

