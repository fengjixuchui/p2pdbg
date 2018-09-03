#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <gdcharconv.h>
#include <mstring.h>
#include "clientview.h"
#include "common.h"
#include "resource.h"
#include "winsize.h"
#include "logic.h"
#include "dbgview.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

#define CONNECT_TIMEOUT     (1000 * 5)
#define TIMER_REFUSH_USERS  (6011)

extern HINSTANCE g_hInst;
static HWND gs_hParent = NULL;
static HWND gs_hwnd = NULL;
static HWND gs_hDbgCode = NULL;
static HWND gs_hStatus = NULL;
static HWND gs_hBtnConnent = NULL;
static ustring gs_wstrDbgCode;

static void _AdjustListctrlWidth(){
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hDbgCode = GetDlgItem(gs_hwnd, IDC_EDIT_DBGCODE);
    CentreWindow(gs_hParent, gs_hwnd);
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_SELECT_STATUS);
    gs_hBtnConnent = GetDlgItem(gs_hwnd, IDC_SELECT_CONNECT);

    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));
    SetWindowTextW(gs_hDbgCode, gs_wstrDbgCode.c_str());
}

static void _OnSize()
{
    _AdjustListctrlWidth();
}

static void _OnSelectClient()
{
    WCHAR wszDbgCode[MAX_PATH] = {0};
    GetWindowTextW(gs_hDbgCode, wszDbgCode, MAX_PATH);
    ustring wstrCode = wszDbgCode;
    wstrCode.trim();

    if (wstrCode.empty())
    {
        return;
    }

    gs_wstrDbgCode = wstrCode;
    //Á¬½Ó¶Ô¶Ë
    if (CWorkLogic::GetInstance()->ConnectDbgClient(WtoU(wstrCode)))
    {
        NotifyConnectSucc();
        EndDialog(gs_hwnd, 0);
    }
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
    int id = LOWORD(wp);
    if (id == IDC_SELECT_CONNECT)
    {
        _OnSelectClient();
    }
}

static void _OnTimer(HWND hwnd, WPARAM wp, LPARAM lp)
{
}

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