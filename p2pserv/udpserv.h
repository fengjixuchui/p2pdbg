#ifndef P2PSERV_H_H_
#define P2PSERV_H_H_
#include <Windows.h>
#include <string>

using namespace std;

typedef void (WINAPI *pfnOnRecv)(const string &strData, const string &strAddr, USHORT uPort);

class CUdpServ {
public:
    CUdpServ();
    bool StartServ(USHORT uPort, pfnOnRecv pfn);
    bool SendTo(const string &strAddr, USHORT uPort, const string &strData);
    string GetLocalIp();

protected:
    string GetLocalIpInternal();
    static DWORD WINAPI WorkThread(LPVOID pParam);

protected:
    bool m_bInit;
    string m_strLocalIp;
    SOCKET m_servSocket;
    USHORT m_uPort;
    HANDLE m_thread;
    pfnOnRecv m_pfn;
};
#endif