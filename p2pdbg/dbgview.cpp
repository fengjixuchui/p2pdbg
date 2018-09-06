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
#include "cmdlancher.h"

using namespace std;

#pragma comment(lib, "comctl32.lib")

struct FtpTransInfo {
    wstring m_wstrDesc;
    wstring m_wstrFilePath;
    bool m_bCompress;
};

#define MSG_CONNECT_SUCC                        (WM_USER + 10011)
#define MSG_FTP_SUCC                            (WM_USER + 10023)
#define MSG_NOTIFYMSG                           (WM_USER + 10024)

#define POPU_MENU_ITEM_CONNECT                  (L"P2P����   Ctrl+U")
#define POPU_MENU_ITEM_CONNECT_ID               (WM_USER + 6050)

#define POPU_MENU_ITEM_COPY                     (L"��������  Ctrl+C")
#define POPU_MENU_ITEM_COPY_ID                  (WM_USER + 6051)

#define POPU_MENU_ITEM_CLEAR                    (L"��ջ���  Ctrl+F")
#define POPU_MENU_ITEM_CLEAR_ID                 (WM_USER + 6052)

#define POPU_MENU_ITEM_SETTING                  (L"���ù���  Ctrl+T")
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

static list<ustring> _CmdListClient(const ustring &wstrCmd, const ustring &wstrParam) {
    list<ustring> result;
    vector<ClientInfo> ClientSet = CWorkLogic::GetInstance()->GetClientSet();
    for (vector<ClientInfo>::const_iterator it = ClientSet.begin() ; it != ClientSet.end() ; it++)
    {
        if (it->m_strClientDesc.find("p2pdbg") != mstring::npos)
        {
            continue;
        }
        ustring wstr = fmt(
            L"�豸��ʶ:%hs �豸����:%hs ip:%hs ����ʱ��:%hs",
            it->m_strUnique.c_str(),
            it->m_strClientDesc.c_str(),
            it->m_strIpInternal.c_str(),
            it->m_strStartTime.c_str()
            );
        result.push_back(wstr);
    }
    return result;
}

static list<ustring> _CmdListCmd(const ustring &wstrCmd, const ustring &wstrParam) {
    list<ustring> result;
    result.push_back("��������");
    list<ustring> tmp = CCmdLancher::GetInstance()->GetLocalCmdList();
    result.insert(result.end(), tmp.begin(), tmp.end());
    result.push_back("���Զ�����");
    tmp = CCmdLancher::GetInstance()->GetRemoteCmdList();
    result.insert(result.end(), tmp.begin(), tmp.end());
    return result;
}

static list<ustring> _CmdClear(const ustring &wstrCmd, const ustring &wstrParam) {
    list<ustring> result;
    SetWindowTextW(gs_hShow, L"");
    return result;
}

static list<ustring> _CmdConnectDbgClient(const ustring &wstrCmd, const ustring &wstrParam) {
    list<ustring> result;
    if (wstrParam.empty())
    {
        result.push_back(L"û���ն˱�ʶ����");
        return result;
    }

    if (CWorkLogic::GetInstance()->ConnectDbgClient(WtoU(wstrParam)))
    {
        result.push_back(L"���ӵ����ն˳ɹ�");
        NotifyConnectSucc();
    }
    else
    {
        result.push_back(L"���ӵ����ն�ʧ��");
    }
    return result;
}

static void _InitLocalCmd() {
    CCmdLancher::GetInstance()->RegisterCmd(L"clt", L"�г����еĵ����ն�", _CmdListClient);
    CCmdLancher::GetInstance()->RegisterCmd(L"cls", L"��յ���ҳ��", _CmdClear);
    CCmdLancher::GetInstance()->RegisterCmd(L"cmd", L"֧�ֵ���������", _CmdListCmd);
    CCmdLancher::GetInstance()->RegisterCmd(L"ct", L"���ӵ����ն�", _CmdConnectDbgClient);
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

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    CentreWindow(NULL, gs_hwnd);
    gs_hShow = GetDlgItem(gs_hwnd, IDC_EDT_SHOW);
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_DBG_STATUS);
    gs_hEdtCmd = GetDlgItem(gs_hwnd, IDC_DBG_CMD);
    gs_hLabel = GetDlgItem(gs_hwnd, IDC_DBG_EDT_LABEL);

    SetWindowTextW(gs_hLabel, L"��������>>");

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
    SendMessageW(gs_hShow, EM_SETLIMITTEXT, 0, 0);

    WCHAR wszPePath[256] = {0};
    GetModuleFileNameW(NULL, wszPePath, 256);

    WCHAR wszVersion[256] = {0};
    GetPeVersionW(wszPePath, wszVersion, 256);
    _AppendText(fmt(L"�汾:%ls", wszVersion));
    _InitLocalCmd();
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
    WCHAR wszCommand[512] = {0};
    GetWindowTextW(gs_hEdtCmd, wszCommand, 512);
    ustring wstrCmd = wszCommand;
    wstrCmd.trim();
    SetWindowTextW(gs_hEdtCmd, L"");

    if (wstrCmd.empty())
    {
        return;
    }

    wstring wstrShow = fmt(L"ִ�� %ls", wstrCmd.c_str());
    _AppendText(wstrShow);
    list<ustring> result = CCmdLancher::GetInstance()->RunCmd(wszCommand);
    ustring wstrShowStr;
    for (list<ustring>::const_iterator it = result.begin() ; it != result.end() ; it++)
    {
        wstrShowStr += it->c_str();
        wstrShowStr += L"\r\n";
    }

    if (result.empty())
    {
        wstrShowStr += L"û���κ�����\r\n";
    }
    _AppendText(wstrShowStr);
}

static void _OnConnectDbgSucc(HWND hwnd, WPARAM wp, LPARAM lp)
{
    ClientInfo CurDbg = CWorkLogic::GetInstance()->GetDbgClient();
    ustring wstrTitle;
    wstrTitle.format(L"p2pdbg-%hs ������...", CurDbg.m_strClientDesc.c_str());
    SetWindowTextW(gs_hwnd, wstrTitle.c_str());
    SendMessage(gs_hShow, WM_VSCROLL, SB_BOTTOM, 0);
}

static void _OnFtpSucc(HWND hwnd, WPARAM wp, LPARAM lp)
{
    FtpTransInfo *pInfo = (FtpTransInfo *)wp;

    if (ustring::npos != pInfo->m_wstrDesc.find(L"logFile"))
    {
        int res = MessageBoxW(gs_hwnd, L"��־�ļ�������ɣ��Ƿ���ز鿴?", L"��־����", MB_OKCANCEL);
        if (res == IDCANCEL)
        {
            return;
        }

        LoadLogData((LPCWSTR)pInfo->m_wstrFilePath.c_str());
        DeleteFileW(pInfo->m_wstrFilePath.c_str());
        ShowLogView(gs_hwnd);
    }
    else
    {
        wstring wtrFilePath = pInfo->m_wstrFilePath;
        if (pInfo->m_bCompress)
        {
            ustring wstrDir = pInfo->m_wstrFilePath;
            size_t pos = wstrDir.rfind(L".");
            if (pos != ustring::npos)
            {
                wstrDir.erase(pos, wstrDir.size() - pos);
            }

            extern ustring g_wstrCisPack;
            ustring wstrCommand = fmt(L"%ls x \"%ls\" -y -aos -o\"%ls\"", g_wstrCisPack.c_str(), pInfo->m_wstrFilePath.c_str(), wstrDir.c_str());
            HANDLE h = ProcExecProcessW(wstrCommand.c_str(), NULL, FALSE);
            WaitForSingleObject(h, INFINITE);
            CloseHandle(h);
            DeleteFileW(pInfo->m_wstrFilePath.c_str());
            wtrFilePath = wstrDir;
        }

        int res = MessageBoxW(gs_hwnd, pInfo->m_wstrDesc.c_str(), L"�ļ��������", MB_OKCANCEL);
        if (res == IDOK)
        {
            ustring cmd;
            cmd.format(L"%ls,\"%ls\"", L"/select", wtrFilePath.c_str());
            ShellExecuteW(NULL, L"open", L"explorer.exe", cmd.c_str(), NULL, SW_SHOWNORMAL);
        }
    }
}

static void _OnNotifyMsg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    MessageBoxW(gs_hwnd, (LPCWSTR)wp, L"��Ϣ", 0);
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
    case MSG_NOTIFYMSG:
        _OnNotifyMsg(hwndDlg, wParam, lParam);
        break;
    case  WM_CLOSE:
        _OnClose(hwndDlg, wParam, lParam);
        break;
    }
    return 0;
}

void NotifyLogFile(LPCWSTR wszDesc, LPCWSTR wszLogFile, bool bCompress)
{
    static FtpTransInfo s_ftpInfo;
    s_ftpInfo.m_wstrDesc = wszDesc;
    s_ftpInfo.m_wstrFilePath = wszLogFile;
    s_ftpInfo.m_bCompress = bCompress;
    PostMessage(gs_hwnd, MSG_FTP_SUCC, (WPARAM)&s_ftpInfo, 0);
}

void NotifyMessage(LPCWSTR wszMsg)
{
    static ustring s_wstrMessage;
    s_wstrMessage = wszMsg;
    PostMessage(gs_hwnd, MSG_NOTIFYMSG, (WPARAM)s_wstrMessage.c_str(), 0);
}

void NotifyConnectSucc()
{
    PostMessage(gs_hwnd, MSG_CONNECT_SUCC, 0, 0);
}

void ShowDbgViw()
{
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_DEBUG), NULL, _DialogProc);
}