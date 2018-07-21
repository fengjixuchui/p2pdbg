#include <Windows.h>
#include <mstring.h>
#include <json/json.h>
#include <gdcharconv.h>
#include "winsize.h"
#include "common.h"
#include "resource.h"

using namespace std;
using namespace Json;

static HWND gs_hParent = NULL;
static HWND gs_hwnd = NULL;
static HWND gs_hFmt = NULL;
static ustring gs_wstrData;

static ustring _GetFmtData()
{
    int iStart = gs_wstrData.find(L"{");
    int iEnd = gs_wstrData.rfind(L"}");

    if (iStart == ustring::npos || iEnd == ustring::npos)
    {
        return gs_wstrData;
    }
    ustring wstrJson = gs_wstrData.substr(iStart, iEnd - iStart + 1);

    Value v;
    Reader().parse(WtoU(wstrJson), v);
    ustring wstr = UtoW(StyledWriter().write(v));
    wstr.repsub(L"\n", L"\r\n");
    return wstr;
}

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
    gs_hwnd = hwnd;
    gs_hFmt = GetDlgItem(gs_hwnd, IDC_EDT_FMT_JSON);
    CentreWindow(gs_hParent, hwnd);
    SetWindowTextW(gs_hFmt, _GetFmtData().c_str());

    extern HINSTANCE g_hInst;
    SendMessageW(gs_hwnd, WM_SETICON, (WPARAM)TRUE, (LPARAM)LoadIconW(g_hInst, MAKEINTRESOURCEW(IDI_MAIN)));

    CTL_PARAMS arry[] =
    {
        {NULL, gs_hFmt, 0, 0, 1, 1}
    };
    SetCtlsCoord(gs_hwnd, arry, RTL_NUMBER_OF(arry));

    RECT rt = {0};
    GetWindowRect(gs_hwnd, &rt);
    SetWindowRange(gs_hwnd, rt.right - rt.left, rt.bottom - rt.top, 0, 0);
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
    case  WM_CLOSE:
        _OnClose(hwndDlg, wParam, lParam);
        break;
    }
    return 0;
}

void ShowFmtView(HWND hParent, LPCWSTR wszData)
{
    extern HINSTANCE g_hInst;
    gs_hParent = hParent;
    gs_wstrData = wszData;
    DialogBoxW(g_hInst, MAKEINTRESOURCEW(IDD_FORMAT), hParent, _DialogProc);
}