#include <Windows.h>
#include <CommCtrl.h>
#include "clientview.h"
#include "common.h"
#include "resource.h"
#include "winsize.h"

#pragma comment(lib, "comctl32.lib")

extern HINSTANCE g_hInst;
static HWND gs_hParent = NULL;
static HWND gs_hwnd = NULL;
static HWND gs_hList = NULL;
static HWND gs_hStatus = NULL;
static HWND gs_hBtnConnent = NULL;

static void _InitListCtrl() {
    ListView_SetExtendedListViewStyle(gs_hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    LVCOLUMNW col;
    memset(&col, 0x00, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 40;
    col.pszText = L"#";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    col.cx = 100;
    col.pszText = L"设备标识";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    col.cx = 100;
    col.pszText = L"设备描述";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    col.cx = 180;
    col.pszText = L"对端地址";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 2, (LPARAM)&col);
}

static void _AdjustListctrlWidth(){
    RECT rt = {0};
    GetWindowRect(gs_hList, &rt);
    int width = (rt.right - rt.left);
    int lastWidth = (width - 40 - 100 - 100 - 30);

    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 0, 40);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 1, 100);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 2, 100);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 3, lastWidth);
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hList = GetDlgItem(gs_hwnd, IDC_SELECT_LIST);
    CentreWindow(NULL, gs_hwnd);
    _InitListCtrl();
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_SELECT_STATUS);
    gs_hBtnConnent = GetDlgItem(gs_hwnd, IDC_SELECT_CONNECT);

    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));
    _AdjustListctrlWidth();
}

static void _OnSize(){
    _AdjustListctrlWidth();
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{}

static void _OnClose(HWND hwnd, WPARAM wp, LPARAM lp)
{
    EndDialog(gs_hwnd, 0);
}

static INT_PTR CALLBACK _DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case  WM_INITDIALOG:
        _OnInitDlg(hwndDlg, wParam, lParam);
        break;
    case  WM_COMMAND:
        _OnCommand(hwndDlg, wParam, lParam);
        break;
    case WM_SIZE:
        _OnSize();
        break;
    case  WM_CLOSE:
        _OnClose(hwndDlg, wParam, lParam);
        break;
    }
    return 0;
}

void ShowClientView(HWND hParent){
    gs_hParent = hParent;
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_SELECT), hParent, _DialogProc);
}