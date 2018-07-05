#ifndef DBGCLIENT_P2PDBG_H_H_
#define DBGCLIENT_P2PDBG_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>

using namespace std;

class CDbgClient;
class CDbgHandler
{
public:
    virtual void onRecvData(CDbgClient *ptr, const string &strData) = 0;
};

class CDbgClient
{
public:
    CDbgClient();
    virtual ~CDbgClient();

    bool InitClient(const string &strServIp, unsigned short uPort, int iTimeOut, CDbgHandler *handler);
    bool SendData(const string &strData);
    bool Close();
    string GetLocalIp();

protected:
    bool Connect(const string &strServIp, unsigned short uPort, int iTimeOut);
    static DWORD WINAPI RecvThread(LPVOID pParam);

protected:
    bool m_bConnect;
    string m_strLocalIp;
    unsigned short m_uLocalPort;
    CDbgHandler *m_handler;
    HANDLE m_hWorkThread;
    SOCKET m_clinetSock;
};
#endif