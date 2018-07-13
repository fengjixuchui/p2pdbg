#include <WinSock2.h>
#include <Windows.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <mstring.h>
#include <gdcharconv.h>
#include "dbgview.h"
#include "logic.h"
#include "common.h"
#include "resource.h"
#include "logview.h"

using namespace std;

HINSTANCE g_hInst = NULL;
ustring g_wstrWorkDir;
ustring g_wstrCisPack;

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
#define MUTEX_INSTANCE           L"89fbaf46-b1a0-42dd-b16a-c0f22222c3d1"

static bool _InitDbgEnv()
{
    WCHAR wszPath[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, wszPath, MAX_PATH);
    PathAppendW(wszPath, L"..\\p2pdbg");
    g_wstrWorkDir = wszPath;
    SHCreateDirectoryExW(NULL, g_wstrWorkDir.c_str(), NULL);
    PathAppendW(wszPath, L"ptool.exe");
    g_wstrCisPack = wszPath;

    if (INVALID_FILE_ATTRIBUTES == GetFileAttributesW(g_wstrCisPack.c_str()))
    {
        //释放压缩工具
        ustring wstrCisPack;
        ReleaseResource(g_wstrCisPack.c_str(), IDR_PEFILE, L"pefile");
    }
    return true;
}

static bool _IsProcRunning()
{
    HANDLE h = OpenMutexW(MUTEX_MODIFY_STATE, FALSE, MUTEX_INSTANCE);

    CloseHandle(h);
    return (h != NULL || (h == NULL && 5 == GetLastError()));
}

int WINAPI WinMain(HINSTANCE hT, HINSTANCE hP, LPSTR szCmdLine, int iShow)
{
    if (_IsProcRunning())
    {
        return 0;
    }

    CreateMutexW(NULL, FALSE, MUTEX_INSTANCE);
    std::locale::global(std::locale(""));
    g_hInst = hT;
    _InitDbgEnv();

    WSADATA wsaData;
    WSAStartup( MAKEWORD(2, 2), &wsaData);

    if (!CWorkLogic::GetInstance()->StartWork())
    {
        MessageBoxW(NULL, L"初始化调试器失败", 0, 0);
        return 0;
    }
    ShowDbgViw();
    return 0;
}