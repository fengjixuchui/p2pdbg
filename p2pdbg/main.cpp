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

static string _GetSerIpFromDomain(const char *domain)
{
    HOSTENT *host_entry = NULL;
    host_entry = gethostbyname(domain);
    if (host_entry != NULL)  
    {
        return fmt(
            "%d.%d.%d.%d",
            (host_entry->h_addr_list[0][0]&0x00ff),
            (host_entry->h_addr_list[0][1]&0x00ff),
            (host_entry->h_addr_list[0][2]&0x00ff),
            (host_entry->h_addr_list[0][3]&0x00ff)
            );
    }
    return "";
}

static Ups *gs_pUpsClient = new Ups();

static DWORD WINAPI _ClientDbgThread(LPVOID pParam)
{
    string strServIp = _GetSerIpFromDomain(TEST_DOMAIN);
    gs_pUpsClient->UpsInit(0, false);

    int iCount = 0;
    char buffer[1024] = {0};
    while (true)
    {
        memset(buffer, 'a', 145);
        wnsprintfA(buffer + 145, 128, " packet %d", iCount);
        gs_pUpsClient->UpsSend(buffer, lstrlenA(buffer));

        iCount++;
        if (iCount >= 5000)
        {
            break;
        }
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    g_hInst = hT;
    CreateThread(NULL, 0, _ClientDbgThread, NULL, 0, NULL);
    MessageBoxW(NULL, L"client", L"test", 0);
    return 0;

    CWorkLogic::GetInstance()->StartWork(NULL);
    ShowDbgViw();
    return 0;
}