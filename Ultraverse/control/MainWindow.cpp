
#include "MainWindow.h"
#include "resource.h"

#include "common/registry.h"
#include "mmDevices.h"


MainWindow::MainWindow(DLGPROC dlgProc)
    : icon_(IDC_MYICON)
    , dlgProc_(dlgProc)
{
}

MainWindow::~MainWindow()
{
    DestroyWindow(hWnd_);
}

void MainWindow::create()
{
    hWnd_ = CreateDialogParamW(hInstance(), MAKEINTRESOURCEW(IDR_MAINFRAME), GetDesktopWindow(), dlgProc_, 0);
    SetWindowTextW(hWnd_, appNameW());

    deviceList_ = GetDlgItem(hWnd_, IDC_DEVICES);
    ListView_SetExtendedListViewStyle(deviceList_, ListView_GetExtendedListViewStyle(deviceList_) | LVS_EX_CHECKBOXES);

    LVCOLUMN column;
    column.mask = LVCF_TEXT;

    column.pszText = (LPWSTR)L"Endpoint";
    ListView_InsertColumn(deviceList_, 0, &column);
    column.pszText = (LPWSTR)L"Device";
    ListView_InsertColumn(deviceList_, 1, &column);
    column.pszText = (LPWSTR)L"Status";
    ListView_InsertColumn(deviceList_, 2, &column);

    updateDeviceInfos();
    updateDeviceInfoList();


}

INT_PTR MainWindow::dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendMessageW(hWnd, WM_SETICON, 0, (LPARAM)icon_.operator HICON());
            return TRUE;

        case WM_COMMAND:
            onButtonClicked(LOWORD(wParam));
            break;

        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE)
            {
                EndDialog(hWnd_, TRUE);
                //PostQuitMessage(0);
                return(TRUE);
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case LVN_ITEMCHANGED:
                    //>onLvnItemChanged((unsigned)((LPNMHDR)lParam)->idFrom, (LPNMLISTVIEW)lParam);
                    return TRUE;
                    break;
            }
            break;
    }

    return FALSE;
}

void MainWindow::onButtonClicked(int id)
{
    switch (id)
    {
        case IDC_BTNUPDATE:
        {
            ListView_DeleteAllItems(deviceList_);
            deviceInfos_.clear();
            updateDeviceInfos();
            updateDeviceInfoList();
        }

        default:
            break;
    }
}

void MainWindow::updateDeviceInfos()
{
    auto devices = Registry::subKeys(L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render");
    for (auto & device : devices)
    {
        auto devInfo = std::make_shared<MMDeviceInfo>(device);
        if (!devInfo->isPresent())
        {
            continue;
        }

        deviceInfos_.push_back(devInfo);
    }
}

void MainWindow::updateDeviceInfoList()
{
    auto defaultDevilce = getDefaultMMDevice(eRender);

    LVITEM item;
    item.iSubItem = 0;
    item.pszText = nullptr;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.stateMask = -1;

    int itemIndex = 0;
    for (auto & device : deviceInfos_)
    {
        std::wstring status;

        item.iItem = itemIndex;
        item.lParam = itemIndex;
        item.pszText = device->endpointName.data();
        item.state = 0;

        if (device->guid == defaultDevilce)
        {
            status += L"default";
            item.state = LVIS_DROPHILITED;
        }
        if (device->mfxInstalled)
        {
            status += (status.empty() ? L", "s : L""s) + L"effect installed";
        }

        ListView_InsertItem(deviceList_, &item);
        ListView_SetItemText(deviceList_, itemIndex, 1, device->deviceName.data());
        ListView_SetItemText(deviceList_, itemIndex, 2, status.data());

        ++itemIndex;
    }

    ListView_SetColumnWidth(deviceList_, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(deviceList_, 1, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(deviceList_, 2, LVSCW_AUTOSIZE_USEHEADER);
}
