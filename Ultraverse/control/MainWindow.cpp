
#include "MainWindow.h"
#include "resource.h"




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
}

INT_PTR MainWindow::dlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            SendMessageW(hWnd, WM_SETICON, 0, (LPARAM)icon_.operator HICON());
            return TRUE;

        case WM_COMMAND:
            //onButtonClicked(LOWORD(wParam))
            break;

        case WM_SYSCOMMAND:
            if (wParam == SC_CLOSE)
            {
                EndDialog(hWnd_, TRUE);
                PostQuitMessage(0);
                return(TRUE);
            }
            break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code)
            {
                case LVN_ITEMCHANGED:
                    //>onLvnItemChanged((unsigned)((LPNMHDR)lParam)->idFrom, (LPNMLISTVIEW)lParam);
                    break;
            }
            break;
    }

    return FALSE;
}
