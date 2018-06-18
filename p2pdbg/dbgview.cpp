#include <Windows.h>
#include <CommCtrl.h>
#include "dbgview.h"
#include "common.h"
#include "resource.h"
#include "winsize.h"

#pragma comment(lib, "comctl32.lib")

#define IDC_STATUS 1011

extern HINSTANCE g_hInst;
static HWND gs_hwnd = NULL;
static HWND gs_hlist = NULL;
static HWND gs_hStatus = NULL;
static HWND gs_hEdtCmd = NULL;
static HWND gs_hBtnRun = NULL;

static void _CreateStatusBar() {
    HWND status = CreateStatusWindowA(WS_CHILD | WS_VISIBLE, NULL, gs_hwnd, IDC_STATUS);
    int wide[3] = {0};
    int length = 0;
    wide[0] = 280;
    wide[1] = wide[0] + 480;
    wide[2]= wide[1] + 360;
    SendMessage(status, SB_SETPARTS, sizeof(wide) / sizeof(int), (LPARAM)(LPINT)wide); 
    gs_hStatus = status;
}

static void _InitListCtrl() {
    ListView_SetExtendedListViewStyle(gs_hlist, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    LVCOLUMNW col;
    memset(&col, 0x00, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 40;
    col.pszText = L"#";
    SendMessageW(gs_hlist, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    col.cx = 100;
    col.pszText = L"时间";
    SendMessageW(gs_hlist, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    col.cx = 480;
    col.pszText = L"调试信息";
    SendMessageW(gs_hlist, LVM_INSERTCOLUMNW, 2, (LPARAM)&col);
}

static void _AdjustListctrlWidth(){
    RECT rt = {0};
    GetWindowRect(gs_hlist, &rt);
    int width = (rt.right - rt.left);
    int lastWidth = (width - 100 - 40 - 30);

    SendMessageW(gs_hlist, LVM_SETCOLUMNWIDTH, 0, 40);
    SendMessageW(gs_hlist, LVM_SETCOLUMNWIDTH, 1, 100);
    SendMessageW(gs_hlist, LVM_SETCOLUMNWIDTH, 2, lastWidth);
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hlist = GetDlgItem(gs_hwnd, IDC_DBG_LIST);
    CentreWindow(NULL, gs_hwnd);
    _InitListCtrl();
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_DBG_STATUS);
    gs_hEdtCmd = GetDlgItem(gs_hwnd, IDC_DBG_CMD);

    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));

    RECT rt = {0};
    GetWindowRect(gs_hwnd, &rt);
    SetWindowRange(gs_hwnd, rt.right - rt.left, rt.bottom - rt.top, 0, 0);

    RECT rtStatus = {0};
    GetWindowRect(gs_hwnd, &rtStatus);
    RECT rtCmd = {0};
    GetWindowRect(gs_hEdtCmd, &rtCmd);
    float f1 = (float)(rtStatus.right - rtStatus.left) / (rt.right - rt.left);
    float f2 = (float)(1 - f1);

    CTL_PARAMS arry[] = {
        {0, gs_hlist, 0, 0, 1, 1},
        {0, gs_hStatus, 0, 1, f1, 0},
        {0, gs_hEdtCmd, f1, 1, f2, 0},
        {IDC_BTN_RUN, 0, 1, 1, 0, 0}
    };
    SetCtlsCoord(gs_hwnd, arry, RTL_NUMBER_OF(arry));
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

void ShowDbgViw(){
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_DEBUG), NULL, _DialogProc);
}