#ifndef P2PDBG_LOGIC_H_H_
#define P2PDBG_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <map>
#include "udpserv.h"

using namespace std;

#define PACKET_STARTMARK    "<Protocol Start>"
#define IP_SERV             "112.232.14.36"
#define PORT_SERV           9951
#define PORT_LOCAL          8807
#define UNIQUE              "9ffab3fb-8621-4582-8937-44af8015a1d8"

struct ClientInfo {
    string m_strUnique;

    string m_strIpInternal;
    USHORT m_uPortInternal;

    string m_strIpExternal;
    USHORT m_uPortExternal;

    ULONG m_uLastHeartbeat;

    ClientInfo() {
        m_uLastHeartbeat = 0;
        m_uPortInternal = 0;
        m_uPortExternal = 0;
    }
};

class CWorkLogic {
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    static void WINAPI OnRecv(const string &strData, const string &strAddr, USHORT uPort);
    void Send(const string &strData, const string &strAddr, USHORT uPort);
    bool Connect(const string &strUnique, DWORD dwTimeOut);

protected:
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    void OnSingleData(const string &strData, const string &strAddr, USHORT uPort);

protected:
    HANDLE m_hP2pNotify;
    string m_strServIp;
    USHORT m_uServPort;
    map<string, ClientInfo> m_clientInfos;
    CUdpServ *m_pUdpServ;
    static CWorkLogic *ms_inst;
};
#endif