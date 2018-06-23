#ifndef P2PSERV_LOGIC_H_H_
#define P2PSERV_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <map>
#include <json/json.h>
#include <LockBase.h>
#include "udpserv.h"

using namespace std;
using namespace Json;

#define PORT_SERV           9692
#define PACKET_STARTMARK    "<Protocol Start>"
#define TIMECOUNT_MAX       (1000 * 30)

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

class CWorkLogic : public CCriticalSectionLockable{
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    bool StartWork(USHORT uLocalPort);
    static void WINAPI OnRecv(const string &strData, const string &strAddr, USHORT uPort);
    void SendTo(const string &strAddr, USHORT uPort, const string &strData);
    string GetLocalIp();

protected:
    void OnClientLogin(const string &strAddr, USHORT uPort, const Value &vJson);
    void OnClientLogout(const string &strAddr, USHORT uPort, const Value &vJson);
    void OnSingleData(const string &strData, const string &strAddr, USHORT uPort);
    void OnGetClient(const string &strAddr, USHORT uPort, const Value &vJson);
    void OnRequestNetPass(const string &strIp, USHORT uPort, const Value &vJson);
    void OnClientHeartbeat(const string &strIp, USHORT uPort, const Value &vJson);
    void OnCheckClientStat();
    static DWORD WINAPI ServWorkThread(LPVOID pParam);

protected:
    bool m_bInit;
    CUdpServ *m_pUdpServ;
    map<string, ClientInfo> m_clientInfos;
    HANDLE m_hWorkThread;
    static CWorkLogic *ms_inst;
};
#endif