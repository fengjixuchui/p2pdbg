#ifndef P2PSERV_H_H_
#define P2PSERV_H_H_
#include <Windows.h>
#include <string>

using namespace std;

typedef void (WINAPI *pfnOnRecv)(const string &strData, const string &strAddr, USHORT uPort);

class CUdpServ {
public:
    CUdpServ();
    bool StartServ(USHORT uLocalPort, pfnOnRecv pfn);
    bool SendTo(const string &strIp, USHORT uPort, const string &strData);

protected:
    static DWORD WINAPI WorkThread(LPVOID pParam);

protected:
    string m_strServIp;
    USHORT m_uServPort;
    string m_strLocalIp;
    USHORT m_uLocalPort;

    bool m_bInit;
    SOCKET m_clientSock;

    HANDLE m_thread;
    pfnOnRecv m_pfn;
};
#endif