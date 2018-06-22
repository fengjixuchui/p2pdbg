#ifndef P2PDBG_LOGIC_H_H_
#define P2PDBG_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <map>
#include <json/json.h>
#include "udpserv.h"

using namespace std;
using namespace Json;

#define PACKET_STARTMARK    "<Protocol Start>"
#define IP_SERV             "10.10.16.38"
#define PORT_SERV           9692
#define PORT_LOCAL          8807
#define UNIQUE              "9ffab3fb-8621-4582-8937-44af8015a1d8"
#define DBGDESC             "p2pdbg"

struct ClientInfo {
    string m_strUnique;
    string m_strClientDesc;

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

class CDataHandler
{
    virtual void OnUpdateClient(const map<string, ClientInfo> &vClients) = 0;
};

class CWorkLogic {
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    static void WINAPI OnRecv(const string &strData, const string &strAddr, USHORT uPort);
    void StartWork(CDataHandler *pHandler);
    void SendTo(const string &strAddr, USHORT uPort, const string &strData);
    bool Connect(const string &strUnique, DWORD dwTimeOut);
    vector<ClientInfo> GetClientList();

protected:
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**注册用户**/
    void RegisterUser();
    /**获取用户列表**/
    void RequestClients();
    void OnSingleData(const string &strData, const string &strAddr, USHORT uPort);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnSendP2pHearbeat();
    void OnGetClients(const Value &vJson);

protected:
    bool m_bInit;
    CDataHandler *m_pHandler;
    HANDLE m_hWorkThread;
    HANDLE m_hP2pNotify;
    string m_strLocalIp;
    USHORT m_uLocalPort;
    string m_strServIp;
    USHORT m_uServPort;

    /**p2p数据**/
    bool m_bP2pStat;
    string m_strP2pIp;
    USHORT m_uP2pPort;
    ClientInfo m_p2pInfo;

    map<string, ClientInfo> m_clientInfos;
    CUdpServ *m_pUdpServ;
    static CWorkLogic *ms_inst;
};
#endif