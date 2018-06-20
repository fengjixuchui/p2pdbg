#include <WinSock2.h>
#include <Windows.h>
#include "dbgview.h"
#include "logic.h"

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

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    g_hInst = hT;
    CWorkLogic::GetInstance()->StartWork(NULL);
    ShowDbgViw();
    return 0;
}