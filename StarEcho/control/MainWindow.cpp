
#include "MainWindow.h"
#include "resource.h"

#include "common/registry.h"
#include "mmDevices.h"
#include <control/control.h>


template<typename T>
std::vector<T> & operator+=(std::vector<T> & v, const T & r)
{
    v.push_back(r);
    return v;
}

std::wstring join(const std::vector<std::wstring> & strings, const std::wstring & delim)
{
    std::wstring r;
    for (auto & s : strings)
    {
        if (!r.empty()) r += delim;
        r += s;
    }
    return r;
}

//

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
    SetWindowTextW(hWnd_, windowNameW());

    deviceList_ = GetDlgItem(hWnd_, IDC_DEVICES);
    btnApply_ = GetDlgItem(hWnd_, IDC_BTNAPPLY);

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
                return TRUE;
            }
            break;

        case WM_NOTIFY:
        {
            auto * hdr = (LPNMHDR)lParam;
            switch (hdr->code)
            {
                case LVN_ITEMCHANGED:
                {
                    auto * lv = (LPNMLISTVIEW)lParam;
                    if ((lv->uChanged & LVIF_STATE) != 0)
                    {
                        updateDeviceFromUi(lv->iItem);
                    }
                    break;
                }
            }
            break;
        }

        case WM_CLOSE:
        case WM_QUIT:
        {
            PostQuitMessage(0);
            break;
        }
    }

    return FALSE;
}

void MainWindow::onButtonClicked(int id)
{
    switch (id)
    {
        case IDC_BTNUPDATE:
        {
            updateDeviceInfos();
            break;
        }

        case IDC_BTNAPPLY:
        {
            try
            {
                bool changesDone = false;
                bool askEnableEnh = true;
                bool doEnableEnh = false;
                for (auto & d : deviceInfos_)
                {
                    if (d->toDoState == MMDeviceInfo::ToDoNothing)
                    {
                        continue;
                    }
                    changesDone = true;

                    d->applyTodo();
                    if (d->enhancementsDisabled)
                    {
                        if (askEnableEnh)
                        {
                            askEnableEnh = false;
                            auto r = MessageBoxW(hWnd_, L"Device(s) have audio enhancements option disabled.\nIn order to audio effects to work enhancements have to be enabled for device(s).\nWould you like to enable it now?",
                                                 appNameW(), MB_ICONQUESTION | MB_YESNO);
                            doEnableEnh = r == IDYES;
                        }
                        if (doEnableEnh)
                        {
                            d->enableEnhancements();
                        }
                    }
                }

                updateDeviceInfoStatus();

                if (changesDone && isUserInAdminGroup())
                {
                    auto r = MessageBoxW(hWnd_, L"Changing audio objects will take place after audio service restart.\nWould you like to restart it now?", 
                                         appNameW(), MB_ICONQUESTION | MB_YESNO);
                    if (r == IDYES)
                    {
                        if (!restartAudioService())
                            MessageBoxW(hWnd_, L"Restarting the service is not gone as planned,\n which may lead to audio playback issues.",
                                appNameW(), MB_ICONWARNING | MB_OK);
                    }
                }
            }
            catch (const WError & e)
            {
                MessageBoxW(hWnd_, (L"Error : "s + e.what()).c_str(), appNameW(), MB_OK | MB_ICONERROR);
                updateDeviceInfos();
                break;
            }

            break;
        }

        default:
            break;
    }
}

void MainWindow::updateDeviceFromUi(int index)
{
    assert(index < deviceInfos_.size());
    if (index >= deviceInfos_.size()) return;

    bool checked = ListView_GetCheckState(deviceList_, index);
    auto device = deviceInfos_[index];

    if (checked && !device->apoMfxInstalled)
    {
        device->toDoState = MMDeviceInfo::ApoToBeInstalled;
    }
    else if (!checked && device->apoMfxInstalled)
    {
        device->toDoState = MMDeviceInfo::ApoToBeRemoved;
    }
    else
    {
        device->toDoState = MMDeviceInfo::ToDoNothing;
    }

    updateDeviceInfoStatus();
}

void MainWindow::updateDeviceInfos()
{
    deviceInfos_ = getPlaybackDevices();

    defaultDevilceGuid_ = getDefaultMMDevice(eRender);
    ListView_DeleteAllItems(deviceList_);

    LVITEM item;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
    item.stateMask = -1;

    int itemIndex = 0;
    for (auto & device : deviceInfos_)
    {
        item.iItem = itemIndex;
        item.lParam = itemIndex;
        item.pszText = device->endpointName.data();
        item.state = 0;

        if (device->guid == defaultDevilceGuid_)
        {
            item.state |= LVIS_DROPHILITED;
        }
        ListView_InsertItem(deviceList_, &item);

        if (device->apoMfxInstalled)
        {
            ListView_SetCheckState(deviceList_, itemIndex, true);
        }

        ListView_SetItemText(deviceList_, itemIndex, 1, device->deviceName.data());

        ++itemIndex;
    }

    EnableWindow(btnApply_, false);

    updateDeviceInfoStatus();
}

void MainWindow::updateDeviceInfoStatus()
{
    bool enableApply = false;

    int itemIndex = 0;
    for (auto & device : deviceInfos_)
    {
        std::vector<std::wstring> status;

        if (device->guid == defaultDevilceGuid_)
        {
            status += L"default device"s;
        }
        if (device->apoMfxInstalled)
        {
            status += L"effect installed"s;
        }
        if (device->enhancementsDisabled)
        {
            status += L"enhancements disabled"s;
        }
        switch (device->toDoState)
        {
            case MMDeviceInfo::ApoToBeInstalled:
                status += L"to be installed"s;
                enableApply = true;
                break;
            case MMDeviceInfo::ApoToBeRemoved:
                status += L"to be removed"s;
                enableApply = true;
                break;

            case MMDeviceInfo::ToDoState::ToDoNothing:
            default:
                break;
        }

        // note the F*g macroses
        auto strStatus = join(status, L", ");
        ListView_SetItemText(deviceList_, itemIndex, 2, strStatus.data());

        ++itemIndex;
    }

    ListView_SetColumnWidth(deviceList_, 0, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(deviceList_, 1, LVSCW_AUTOSIZE);
    ListView_SetColumnWidth(deviceList_, 2, LVSCW_AUTOSIZE_USEHEADER);

    EnableWindow(btnApply_, enableApply);
}
