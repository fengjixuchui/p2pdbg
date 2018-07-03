#include <WinSock2.h>
#include <Windows.h>
#include <Shlwapi.h>
#include <gdcharconv.h>
#include "dbgview.h"
#include "logic.h"
#include "ups.h"

HINSTANCE g_hInst = NULL;

#if defined _M_IX86
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#define TEST_IP_SERV1            "112.232.14.36"
#define TEST_IP_SERV2            "192.168.1.101"
#define TEST_IP_SERV3            "10.10.16.38"
#define TEST_DOMAIN              "p2pdbg.picp.io"
#define TEST_PORT_LOCAL          9977
#define TEST_PORT_SERV           9971

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    g_hInst = hT;
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    CWorkLogic::GetInstance()->StartWork();
    ShowDbgViw();
    return 0;
}