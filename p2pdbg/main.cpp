#include <WinSock2.h>
#include <Windows.h>
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

#define TEST_IP_SERV             "10.10.16.38"
#define TEST_PORT_LOCAL          9977
#define TEST_PORT_SERV           9971

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    string aaa = "1234";
    string bbb = aaa.substr(0, aaa.size());

    g_hInst = hT;
    Ups *client = new Ups();
    client->UpsInit(TEST_PORT_LOCAL, false);

    for (int i = 0 ; i < 100 ; i++)
    {
        client->UpsSend(TEST_IP_SERV, TEST_PORT_SERV, "11111", 6);
        client->UpsSend(TEST_IP_SERV, TEST_PORT_SERV, "22222", 6);
    }
    MessageBoxW(NULL, L"client", L"test", 0);
    return 0;

    CWorkLogic::GetInstance()->StartWork(NULL);
    ShowDbgViw();
    return 0;
}