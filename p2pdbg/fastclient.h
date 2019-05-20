#ifndef FASTCLIENT_P2PDBG_H_H_
#define FASTCLIENT_P2PDBG_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>

using namespace std;

/**
短连接客户端模式
*/
class CFastClient {
public:
    CFastClient();
    bool Connect(const string &strIp, unsigned short uPort, int iTimeOut);
    bool SendData(const string &strData);
    bool Close();
    string RecvData(int iTimeOut);

protected:
    SOCKET m_socket;
};
#endif