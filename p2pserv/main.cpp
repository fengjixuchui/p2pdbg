#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <gdcharconv.h>
#include "udpserv.h"
#include "logic.h"
#include "ups.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "shlwapi.lib")

#if defined _M_IX86
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

static void _OnInitDlg(HWND hwnd, WPARAM wp, LPARAM lp)
{
}

static void _OnCommand(HWND hwnd, WPARAM wp, LPARAM lp)
{}

static void _OnClose(HWND hwnd, WPARAM wp, LPARAM lp)
{
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

#define TEST_PORT_LOCAL     9971

static Ups *gs_pUpsServ = new Ups();

static DWORD WINAPI _DbgThread(LPVOID pParam)
{
    int iCount = 0;
    while (true) {
        string strIp;
        USHORT uPort;
        string strData;
        int iSize = gs_pUpsServ->UpsRecv(strIp, uPort, strData);

        string strDbg = fmt("recv data:%hs\n", strData.c_str());
        OutputDebugStringA(strDbg.c_str());
        iCount++;
    }
    return 0;
}

unsigned short _GetMagicNumber()
{
    DWORD dw = 0x1122;
    unsigned short hight = (dw & 0xff);
    unsigned short low = (hight ^ MAGIC_SEED);
    unsigned short uMagic = ((hight << 8) | low);
    return uMagic;
}

bool _IsValidMagic(unsigned short uMagic)
{
    unsigned short hight = ((uMagic >> 8) & 0xff);
    unsigned short low = (uMagic & 0xff);
    return (low == ((hight ^ MAGIC_SEED) & 0xff));
}

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    unsigned short aa = _GetMagicNumber();
    bool b = _IsValidMagic(aa);

    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    /**
    CWorkLogic::GetInstance()->StartWork(PORT_SERV);
    */
    gs_pUpsServ->UpsInit(TEST_PORT_LOCAL, false);
    CreateThread(NULL, 0, _DbgThread, NULL, 0, NULL);

    char szInfo[128] = {0};
    wnsprintfA(szInfo, 128, "%d", TEST_PORT_LOCAL);
    MessageBoxA(0, "udp serv start...", szInfo, 0);
    return 0;
}