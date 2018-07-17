#include <WinSock2.h>
#include <Windows.h>
#include <CommCtrl.h>
#include <string>
#include <mstring.h>
#include <gdcharconv.h>
#include "dbgview.h"
#include "common.h"
#include "resource.h"
#include "winsize.h"
#include "clientview.h"
#include "logic.h"
#include "logview.h"

using namespace std;

#pragma comment(lib, "comctl32.lib")

#define MSG_CONNECT_SUCC                        (WM_USER + 10011)
#define MSG_FTP_SUCC                            (WM_USER + 10023)

#define POPU_MENU_ITEM_CONNECT                  (L"P2P连接   Ctrl+U")
#define POPU_MENU_ITEM_CONNECT_ID               (WM_USER + 6050)

#define POPU_MENU_ITEM_COPY                     (L"复制数据  Ctrl+C")
#define POPU_MENU_ITEM_COPY_ID                  (WM_USER + 6051)

#define POPU_MENU_ITEM_CLEAR                    (L"清空缓存  Ctrl+F")
#define POPU_MENU_ITEM_CLEAR_ID                 (WM_USER + 6052)

#define POPU_MENU_ITEM_SETTING                  (L"设置规则  Ctrl+T")
#define POPU_MENU_ITEM_SETTING_ID               (WM_USER + 6053)

#define MSG_EXEC_COMMAND                        (WM_USER + 6061)

extern HINSTANCE g_hInst;
static HWND gs_hwnd = NULL;
static HWND gs_hShow = NULL;
static HWND gs_hStatus = NULL;
static HWND gs_hEdtCmd = NULL;
static HWND gs_hBtnRun = NULL;
static HWND gs_hLabel = NULL;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);
static PWIN_PROC gs_pfnCommandProc = NULL;

static LRESULT CALLBACK _CommandProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_CHAR == msg)
    {
        if (0x0d == wp)
        {
            SendMessageW(gs_hwnd, MSG_EXEC_COMMAND, 0, 0);
        }
    }

    return CallWindowProc(gs_pfnCommandProc, hwnd, msg, wp, lp);
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    CentreWindow(NULL, gs_hwnd);
    gs_hShow = GetDlgItem(gs_hwnd, IDC_EDT_SHOW);
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_DBG_STATUS);
    gs_hEdtCmd = GetDlgItem(gs_hwnd, IDC_DBG_CMD);
    gs_hLabel = GetDlgItem(gs_hwnd, IDC_DBG_EDT_LABEL);

    SetWindowTextW(gs_hLabel, L"调试命令>>");

    SetFocus(gs_hEdtCmd);
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
        {0, gs_hShow, 0, 0, 1, 1},
        {0, gs_hStatus, 0, 1, f1, 0},
        {0, gs_hEdtCmd, f1, 1, f2, 0},
        {0, gs_hLabel, 0, 1, 0, 0},
        {0, gs_hEdtCmd, 0, 1, 1, 0}
    };
    SetCtlsCoord(gs_hwnd, arry, RTL_NUMBER_OF(arry));
    gs_pfnCommandProc = (PWIN_PROC)SetWindowLongPtr(gs_hEdtCmd, GWLP_WNDPROC, (LONG_PTR)_CommandProc);

    SendMessageW(gs_hEdtCmd, EM_SETLIMITTEXT, 0, 0);
}

static void _AppendText(const wstring &wstr)
{
    wstring wstrData = wstr;
    wstrData += L"\r\n";
    int iLength = GetWindowTextLengthW(gs_hShow);
    SendMessageW(gs_hShow, EM_SETSEL, iLength, iLength);
    SendMessageW(gs_hShow, EM_REPLACESEL, TRUE, (LPARAM)wstrData.c_str());
    InvalidateRect(gs_hShow, NULL, TRUE);
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
    DWORD id = LOWORD(wp);
    switch (id) {
        case IDR_MENU_DEVICE:
            ShowClientView(gs_hwnd);
            break;
        case IDR_SUB_LOG:
            ShowLogView(gs_hwnd);
            break;
        default:
            break;
    }
}

static void _OnClose(HWND hwnd, WPARAM wp, LPARAM lp)
{
    EndDialog(gs_hwnd, 0);
}

static void _OnRunCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
    if (!CWorkLogic::GetInstance()->IsConnectDbg())
    {
        _AppendText(L"尚未连接调试终端");
        return;
    }

    WCHAR wszCommand[512] = {0};
    GetWindowTextW(gs_hEdtCmd, wszCommand, 512);
    ustring wstrCmd = wszCommand;
    wstrCmd.trim();
    SetWindowTextW(gs_hEdtCmd, L"");

    if (wstrCmd.empty())
    {
        return;
    }

    wstring wstrShow = fmt(L"执行 %ls", wstrCmd.c_str());
    _AppendText(wstrShow);
    ustring wstrReply = CWorkLogic::GetInstance()->ExecCmd(wstrCmd, 5000);

    Value vResult;
    Reader().parse(WtoU(wstrReply), vResult);

    ustring wstrReplyShow;
    if (vResult.type() == arrayValue)
    {
        for (int i = 0 ; i < (int)vResult.size() ; i++)
        {
            wstrReplyShow += UtoW(vResult[i].asString());
            wstrReplyShow += L"\r\n";
        }

        if (wstrReplyShow.empty())
        {
            wstrReplyShow = L"没有任何数据";
        }
    }
    else
    {
        wstrReplyShow = L"等待超时";
    }
    _AppendText(wstrReplyShow);
}

static void _OnConnectDbgSucc(HWND hwnd, WPARAM wp, LPARAM lp)
{
    ClientInfo CurDbg = CWorkLogic::GetInstance()->GetDbgClient();
    ustring wstrTitle;
    wstrTitle.format(
        L"p2pdbg-%hs %hs:%d连接中...",
        CurDbg.m_strClientDesc.c_str(),
        CurDbg.m_strIpInternal.c_str(),
        CurDbg.m_uPortInternal
        );
    SetWindowTextW(gs_hwnd, wstrTitle.c_str());
    SendMessage(gs_hShow, WM_VSCROLL, SB_BOTTOM, 0);
}

static void _OnFtpSucc(HWND hwnd, WPARAM wp, LPARAM lp)
{
    ustring wstrFile = (LPCWSTR)wp;
    ustring wstrDesc = (LPCWSTR)lp;

    if (ustring::npos != wstrDesc.find(L"logFile"))
    {
        int res = MessageBoxW(gs_hwnd, L"日志文件接收完成，是否加载查看?", L"日志数据", MB_OKCANCEL);
        if (res == IDCANCEL)
        {
            return;
        }

        LoadLogData((LPCWSTR)wp);
        ShowLogView(gs_hwnd);
    }
    else
    {
        ustring wstrDir = wstrFile;
        size_t pos = wstrDir.rfind(L".");
        if (pos != ustring::npos)
        {
            wstrDir.erase(pos, wstrDir.size() - pos);
        }

        extern ustring g_wstrCisPack;
        ustring wstrCommand = fmt(L"%ls x \"%ls\" -y -aos -o\"%ls\"", g_wstrCisPack.c_str(), wstrFile.c_str(), wstrDir.c_str());
        HANDLE h = ProcExecProcessW(wstrCommand.c_str(), NULL, FALSE);
        WaitForSingleObject(h, INFINITE);
        CloseHandle(h);

        int res = MessageBoxW(gs_hwnd, L"异常信息接收完成，是否加载查看?", L"异常信息", MB_OKCANCEL);
        if (res == IDOK)
        {
            ustring cmd;
            cmd.format(L"%ls,\"%ls\"", "/select", wstrDir.c_str());
            ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
        }
    }
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
    case MSG_CONNECT_SUCC:
        _OnConnectDbgSucc(hwndDlg, wParam, lParam);
        break;
    case MSG_FTP_SUCC:
        _OnFtpSucc(hwndDlg, wParam, lParam);
        break;
    case MSG_EXEC_COMMAND:
        _OnRunCommand(hwndDlg, wParam, lParam);
        break;
    case  WM_CLOSE:
        _OnClose(hwndDlg, wParam, lParam);
        break;
    }
    return 0;
}

void NotifyLogFile(LPCWSTR wszDesc, LPCWSTR wszLogFile)
{
    static ustring s_wstrDesc;
    static ustring s_wstrLogFile;
    s_wstrDesc = wszDesc;
    s_wstrLogFile = wszLogFile;
    PostMessage(gs_hwnd, MSG_FTP_SUCC, (WPARAM)s_wstrLogFile.c_str(), (LPARAM)s_wstrDesc.c_str());
}

void NotifyConnectSucc()
{
    PostMessage(gs_hwnd, MSG_CONNECT_SUCC, 0, 0);
}

void ShowDbgViw()
{
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_DEBUG), NULL, _DialogProc);
}