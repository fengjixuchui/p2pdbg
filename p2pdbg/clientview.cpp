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
static HWND gs_hList = NULL;
static HWND gs_hStatus = NULL;
static HWND gs_hBtnConnent = NULL;
static vector<ClientInfo> gs_vClients;

static void _RefushListCtrl()
{
    gs_vClients = CWorkLogic::GetInstance()->GetClientList();
    PostMessageW(gs_hList, LVM_SETITEMCOUNT, gs_vClients.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    PostMessageW(gs_hList, LVM_REDRAWITEMS, 0, gs_vClients.size());
}

static void _InitListCtrl() {
    ListView_SetExtendedListViewStyle(gs_hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    LVCOLUMNW col;
    memset(&col, 0x00, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 40;
    col.pszText = L"#";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    col.cx = 220;
    col.pszText = L"设备标识";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    col.cx = 180;
    col.pszText = L"设备描述";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 2, (LPARAM)&col);

    col.cx = 120;
    col.pszText = L"对端地址1";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 3, (LPARAM)&col);

    col.cx = 120;
    col.pszText = L"对端地址2";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 4, (LPARAM)&col);
}

static void _AdjustListctrlWidth(){
    RECT rt = {0};
    GetWindowRect(gs_hList, &rt);
    int width = (rt.right - rt.left);
    int lastWidth = (width - 40 - 220 - 180 - 120 - 30);

    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 0, 40);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 1, 220);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 2, 180);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 3, 120);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 4, lastWidth);
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hList = GetDlgItem(gs_hwnd, IDC_SELECT_LIST);
    CentreWindow(gs_hParent, gs_hwnd);
    _InitListCtrl();
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_SELECT_STATUS);
    gs_hBtnConnent = GetDlgItem(gs_hwnd, IDC_SELECT_CONNECT);

    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));
    _AdjustListctrlWidth();
    _RefushListCtrl();

    SetTimer(hwnd, TIMER_REFUSH_USERS, 1000, NULL);
}

static void _OnSize()
{
    _AdjustListctrlWidth();
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
    int id = LOWORD(wp);
    if (id == IDC_SELECT_REFUSH)
    {
        _RefushListCtrl();
    }
    else if (id == IDC_SELECT_CONNECT)
    {
        int pos = SendMessageW(gs_hList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

        if (pos < 0)
        {
            SetWindowTextW(gs_hStatus, L"请选择一个终端");
            return;
        }
        ClientInfo client = gs_vClients[pos];
        if (client.m_strUnique == CWorkLogic::GetInstance()->GetDbgUnique())
        {
            SetWindowTextW(gs_hStatus, L"不能自己连接自己");
            return;
        }

        if (client.m_strClientDesc.find("p2pdbg") != mstring::npos)
        {
            SetWindowTextW(gs_hStatus, L"对端不能是调试器");
            return;
        }

        //连接对端
        if (CWorkLogic::GetInstance()->ConnectReomte(client.m_strUnique))
        {
            SetWindowTextW(gs_hStatus, L"连接对端成功");
            NotifyConnectSucc();
            EndDialog(gs_hwnd, 0);
        }
    }
}

static VOID _OnGetListCtrlDisplsy(IN OUT NMLVDISPINFOW* ptr)
{
    int iItm = ptr->item.iItem;
    int iSub = ptr->item.iSubItem;

    do
    {
        ClientInfo info;
        {
            if (iItm >= (int)gs_vClients.size())
            {
                break;
            }

            info = gs_vClients[iItm];
        }

        static ustring s_wstrBuf;
        switch (iSub)
        {
        case  0:
            {
                char buffer[64] = {0};
                wnsprintfA(buffer, 64, "%d", iItm);
                s_wstrBuf = buffer;
            }
            break;
        case 1:
            s_wstrBuf = UtoW(info.m_strUnique);
            break;
        case 2:
            s_wstrBuf = UtoW(info.m_strClientDesc);
            break;
        case 3:
            s_wstrBuf = UtoW(fmt("%hs:%d", info.m_strIpInternal.c_str(), info.m_uPortInternal));
            break;
        case 4:
            s_wstrBuf = UtoW(fmt("%hs:%d", info.m_strIpExternal.c_str(), info.m_uPortExternal));
            break;
        default:
            break;
        }
        ptr->item.pszText = (LPWSTR)s_wstrBuf.c_str();
    } while (FALSE);
}

static void _OnNotify(HWND hwnd, WPARAM wp, LPARAM lp)
{
    switch (((LPNMHDR) lp)->code)
    {
    case LVN_GETDISPINFO:
        {
            NMLVDISPINFO* ptr = NULL;
            ptr = (NMLVDISPINFOW *)lp;

            if (ptr->hdr.hwndFrom == gs_hList)
            {
                _OnGetListCtrlDisplsy(ptr);
            }
        }
        break;
    default:
        break;
    }
}

static void _OnTimer(HWND hwnd, WPARAM wp, LPARAM lp)
{
    if (wp == TIMER_REFUSH_USERS)
    {
        _RefushListCtrl();
    }
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
    case WM_TIMER:
        _OnTimer(hwndDlg, wParam, lParam);
        break;
    case WM_SIZE:
        _OnSize();
        break;
    case WM_NOTIFY:
        _OnNotify(hwndDlg, wParam, lParam);
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