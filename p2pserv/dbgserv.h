#ifndef DBGSERV_P2PSERV_H_H_
#define DBGSERV_P2PSERV_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <vector>

using namespace std;

struct DbgUserInfo
{
    string m_strUnique;     //用户标识
    SOCKET m_sock;          //用户通信套接字
};

class DbgServHandler
{
public:
    virtual bool OnSocketAccept(SOCKET sock) = 0;
    virtual bool OnDataRecv(SOCKET sock, const string &strRecv, string &strResp) = 0;
    virtual bool OnSocketClose(SOCKET sock) = 0;
};

class CDbgServ
{
public:
    CDbgServ();
    virtual ~CDbgServ();

    bool InitDbgServ(unsigned short uLocalPort, DbgServHandler *handler);
    bool StopDbgServ();
    bool SendData(SOCKET sock, const string &strData);

protected:
    bool SetKeepAlive();
    static DWORD WINAPI WorkThread(LPVOID pParam);

protected:
    bool m_bInit;
    HANDLE m_hWorkThread;
    DbgServHandler *m_handler;
    SOCKET m_servSock;
    vector<SOCKET> m_clientSet;
};
#endif