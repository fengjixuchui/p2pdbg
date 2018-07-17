#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <CommCtrl.h>
#include <fstream>
#include <list>
#include <gdcharconv.h>
#include <mstring.h>
#include "clientview.h"
#include "common.h"
#include "resource.h"
#include "winsize.h"
#include "logic.h"
#include "dbgview.h"
#include "fmtview.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace std;

#define MSG_ACTIVITE    (WM_USER + 4011)
#define MSG_SET_FILTER  (WM_USER + 5011)

#define ID_MENU_SAVE    10011
#define ID_MENU_COPY    10012
#define ID_MUNU_FIND    10013
#define ID_MENU_FORMAT  10014

struct LogDataInfo
{
    ustring m_wstrLogTime;
    ustring m_wstrContent;
};

extern HINSTANCE g_hInst;
static HWND gs_hParent = NULL;
static HWND gs_hwnd = NULL;
static HWND gs_hList = NULL;
static HWND gs_hFilter = NULL;
static HWND gs_hStatus = NULL;

static vector<ustring> gs_vFilterSet;
static vector<LogDataInfo *> gs_LogInfo;
static vector<LogDataInfo *> gs_ShowInfo;

static CCriticalSectionLockable gs_lock;

typedef LRESULT (CALLBACK *PWIN_PROC)(HWND, UINT, WPARAM, LPARAM);
static PWIN_PROC gs_pfnCommandProc = NULL;

static LRESULT CALLBACK _CommandProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    if (WM_CHAR == msg)
    {
        if (0x0d == wp)
        {
            SendMessageW(gs_hwnd, MSG_SET_FILTER, 0, 0);
        }
    }

    return CallWindowProc(gs_pfnCommandProc, hwnd, msg, wp, lp);
}

static void _RefushListCtrl()
{
    CScopedLocker lock(&gs_lock);
    PostMessageW(gs_hList, LVM_SETITEMCOUNT, gs_ShowInfo.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    PostMessageW(gs_hList, LVM_REDRAWITEMS, 0, gs_ShowInfo.size());
}

static void _InitListCtrl() {
    ListView_SetExtendedListViewStyle(gs_hList, LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
    LVCOLUMNW col;
    memset(&col, 0x00, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = 60;
    col.pszText = L"#";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    col.cx = 460;
    col.pszText = L"日志信息";
    SendMessageW(gs_hList, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);
}

static void _AdjustListctrlWidth(){
    RECT rt = {0};
    GetWindowRect(gs_hList, &rt);
    int width = (rt.right - rt.left);
    int lastWidth = (width - 60 - 40);

    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 0, 60);
    SendMessageW(gs_hList, LVM_SETCOLUMNWIDTH, 1, lastWidth);
}

static void _UpdateStatus()
{
    SYSTEMTIME time = {0};
    GetLocalTime(&time);
    ustring wstrTime = fmt(
        L"%04d-%02d-%02d %02d:%02d:%02d %03d",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        time.wMilliseconds
        );

    ustring wstrShow = fmt(L"加载时间:%ls 数据数量:%d 符合规则:%d", wstrTime.c_str(), gs_LogInfo.size(), gs_ShowInfo.size());
    SetWindowTextW(gs_hStatus, wstrShow.c_str());
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hList = GetDlgItem(gs_hwnd, IDC_LOG_LIST);
    gs_hFilter = GetDlgItem(gs_hwnd, IDC_EDT_LOG_FILTER);
    gs_hStatus = GetDlgItem(gs_hwnd, IDC_EDT_LOG_STATUS);

    CentreWindow(gs_hParent, gs_hwnd);
    _InitListCtrl();

    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));
    _AdjustListctrlWidth();

    CTL_PARAMS arry[] = {
        {0, gs_hList, 0, 0, 1, 1},
        {0, gs_hFilter, 0, 0, 1, 0},
        {0, gs_hStatus, 0, 1, 1, 0}
    };
    SetCtlsCoord(gs_hwnd, arry, RTL_NUMBER_OF(arry));

    RECT rt = {0};
    GetWindowRect(hwnd, &rt);
    SetWindowRange(hwnd, rt.right - rt.left, rt.bottom - rt.top, 0, 0);

    gs_pfnCommandProc = (PWIN_PROC)SetWindowLongPtr(gs_hFilter, GWLP_WNDPROC, (LONG_PTR)_CommandProc);
    _RefushListCtrl();
    _UpdateStatus();
}

static void _OnSize()
{
    _AdjustListctrlWidth();
}

static void _OnCopyData()
{
    list<int> vSelList;
    int pos = -1;
    while (TRUE)
    {
        pos = SendMessageW(gs_hList, LVM_GETNEXTITEM, pos, LVNI_SELECTED);
        if (-1 == pos)
        {
            break;
        }
        vSelList.push_back(pos);
    }

    mstring strContent;
    {
        CScopedLocker lock(&gs_lock);
        for (list<int>::const_iterator it = vSelList.begin() ; it != vSelList.end() ; it++)
        {
            if (strContent.size())
            {
                strContent += L"\r\n";
            }
            strContent += gs_LogInfo[*it]->m_wstrContent;
        }
    }

    if (strContent.empty())
    {
        return;
    }
    HGLOBAL hBuffer = NULL;
    do 
    {
        if (!OpenClipboard(NULL))
        {
            break;
        }
        EmptyClipboard();
        hBuffer = GlobalAlloc(GMEM_MOVEABLE, strContent.size() + 16);
        if (!hBuffer)
        {
            break;
        }
        LPSTR pBuffer = (LPSTR)GlobalLock(hBuffer);
        if (!pBuffer)
        {
            break;
        }
        lstrcpynA(pBuffer, strContent.c_str(), strContent.size() + 16);
        GlobalUnlock(hBuffer);
        SetClipboardData(CF_TEXT, hBuffer);
    } while (FALSE);
    if (hBuffer)
    {
        GlobalFree(hBuffer);
    }
    CloseClipboard();
    return;
}

static void _OnFormatData()
{
    int pos = SendMessageW(gs_hList, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
    if (-1 == pos)
    {
        return;
    }

    ustring wstr;
    {
        CScopedLocker lock(&gs_lock);
        wstr = gs_ShowInfo[pos]->m_wstrContent;
    }
    ShowFmtView(gs_hwnd, wstr.c_str());
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{
    int id = LOWORD(wp);
    switch (id) 
    {
    case ID_MENU_COPY:
        _OnCopyData();
        break;
    case ID_MENU_SAVE:
        break;
    case ID_MENU_FORMAT:
        _OnFormatData();
        break;
    case ID_MUNU_FIND:
        break;
    default:
        break;
    }
}

static VOID _OnGetListCtrlDisplsy(IN OUT NMLVDISPINFOW* ptr)
{
    int iItm = ptr->item.iItem;
    int iSub = ptr->item.iSubItem;

    do
    {
        CScopedLocker lock(&gs_lock);
        LogDataInfo *pLogInfo = NULL;
        {
            if (iItm >= (int)gs_ShowInfo.size())
            {
                break;
            }

            pLogInfo = gs_ShowInfo[iItm];
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
            s_wstrBuf = pLogInfo->m_wstrContent;
            break;
        }
        ptr->item.pszText = (LPWSTR)s_wstrBuf.c_str();
    } while (FALSE);
}

static void _OnRButtonUp(HWND hwnd, WPARAM wp, LPARAM lp)
{
    POINT pt = {0}; 
    GetCursorPos(&pt);
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_ENABLED, ID_MUNU_FIND,     L"查找数据   CTRL+F");
    AppendMenuW(menu, MF_ENABLED, ID_MENU_COPY,     L"复制数据   CTRL+L");
    AppendMenuW(menu, MF_ENABLED, ID_MENU_FORMAT,   L"格式化查看 CTRL+U");
    TrackPopupMenu(menu, TPM_CENTERALIGN, pt.x, pt.y, 0, gs_hwnd, 0);
    DestroyMenu(menu);
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
    case NM_RCLICK:
        _OnRButtonUp(hwnd, 0, 0);
        break;
    default:
        break;
    }
}

static void _OnClose(HWND hwnd, WPARAM wp, LPARAM lp)
{
    EndDialog(gs_hwnd, 0);
}

static void _OnFilter(HWND hwnd, WPARAM wp, LPARAM lp)
{
    WCHAR wszFilter[MAX_PATH] = {0};
    GetWindowTextW(gs_hFilter, wszFilter, MAX_PATH);
    ustring wstr = wszFilter;
    wstr.trim();
    size_t last = 0;
    size_t pos = 0;
    gs_vFilterSet.clear();
    ustring wstrSub;
    while (pos = wstr.find(L";", last))
    {
        if (pos == ustring::npos)
        {
            break;
        }

        if (pos > last)
        {
            wstrSub = wstr.substr(last, pos - last);
            wstrSub.trim();

            if (!wstrSub.empty())
            {
                gs_vFilterSet.push_back(wstrSub);
            }
        }
        last = pos + 1;
    }

    if (wstr.size() > last + 1)
    {
        wstrSub = wstr.substr(last, wstr.size() - last);
        wstrSub.trim();

        if (!wstrSub.empty())
        {
            gs_vFilterSet.push_back(wstrSub);
        }
    }

    gs_ShowInfo.clear();
    if (gs_vFilterSet.empty())
    {
        gs_ShowInfo = gs_LogInfo;
    }
    else
    {
        for (vector<LogDataInfo *>::const_iterator it = gs_LogInfo.begin() ; it != gs_LogInfo.end() ; it++)
        {
            LogDataInfo *ptr = *it;
            vector<ustring>::const_iterator ij;
            for (ij = gs_vFilterSet.begin() ; ij != gs_vFilterSet.end() ; ij++)
            {
                if (ptr->m_wstrContent.find_in_rangei(ij->c_str()) == ustring::npos)
                {
                    break;
                }
            }

            if (ij == gs_vFilterSet.end())
            {
                gs_ShowInfo.push_back(ptr);
            }
        }
    }
    _RefushListCtrl();
    _UpdateStatus();
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
    case MSG_ACTIVITE:
        break;
    case MSG_SET_FILTER:
        _OnFilter(hwndDlg, wParam, lParam);
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

//字符串提取其中的时间
static ustring _GetTimeFromStr(ustring &wstr)
{
    SYSTEMTIME time = {0};
    GetLocalTime(&time);
    ustring wstrYear = fmt(L"%d", time.wYear);
    size_t iStartPos = wstr.find(wstrYear);
    size_t iEndPos = wstr.find_in_range(L" ", iStartPos, 32);
    if (iEndPos != ustring::npos)
    {
        iEndPos = wstr.find_in_range(L" ", iEndPos + 3, 32);
    }

    if (iStartPos == ustring::npos || iEndPos == ustring::npos)
    {
        return L"";
    }
    return wstr.substr(iStartPos, iEndPos - iStartPos);
}

static void _ReleaseCache()
{
    CScopedLocker lock(&gs_lock);
    for (vector<LogDataInfo *>::const_iterator it = gs_LogInfo.begin() ; it != gs_LogInfo.end() ; it++)
    {
        delete *it;
    }
    gs_LogInfo.clear();
    gs_ShowInfo.clear();
}

static bool _LoadSingleFile(LPCWSTR wszFile)
{
    fstream fp(WtoA(wszFile).c_str());
    if (!fp.is_open())
    {
        return false;
    }

    {
        CScopedLocker lock(&gs_lock);
        string strLine;
        while (getline(fp, strLine))
        {
            LogDataInfo *ptr = new LogDataInfo();
            ptr->m_wstrContent = UtoW(strLine);
            ptr->m_wstrLogTime = _GetTimeFromStr(ptr->m_wstrContent);
            gs_LogInfo.push_back(ptr);
        }
    }
    return true;
}

static BOOL WINAPI _FileHandlerW(LPCWSTR wszFile, void *pParam)
{
    _LoadSingleFile(wszFile);
    return TRUE;
}

//加载日志文件
bool LoadLogData(LPCWSTR wszZipFile)
{
    extern ustring g_wstrCisPack;
    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(wszZipFile))
    {
        return false;
    }

    ustring wstrDir = wszZipFile;
    size_t pos = wstrDir.rfind(L".");
    if (pos != ustring::npos)
    {
        wstrDir.erase(pos, wstrDir.size() - pos);
    }
    ustring wstrCommand = fmt(L"%ls x \"%ls\" -y -aos -o\"%ls\"", g_wstrCisPack.c_str(), wszZipFile, wstrDir.c_str());
    HANDLE h = ProcExecProcessW(wstrCommand.c_str(), NULL, FALSE);
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);

   _ReleaseCache();
    //加载日志文件
    FileEnumFileW(wstrDir.c_str(), TRUE, _FileHandlerW, NULL);
    _OnFilter(0, 0, 0);
    _RefushListCtrl();
    return true;
}

void ShowLogView(HWND hParent)
{
    if (IsWindow(gs_hwnd))
    {
        SendMessageW(gs_hwnd, MSG_ACTIVITE, 0, 0);
    }
    else
    {
        gs_hParent = hParent;
        DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_LOGVIEW), hParent, _DialogProc);
    }
}