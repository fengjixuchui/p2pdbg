#ifndef P2PDBG_LOGIC_H_H_
#define P2PDBG_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <map>
#include <json/json.h>
#include "dbgclient.h"
#include "LockBase.h"

using namespace std;
using namespace Json;

#define IP_SERV             "10.10.16.38"
#define PORT_SERV           9971
#define PORT_LOCAL          8807
#define UNIQUE              "9ffab3fb-8621-4582-8937-44af8015a1d8"
#define DBGDESC             "p2pdbg"

struct ClientInfo
{
    string m_strUnique;
    string m_strClientDesc;

    string m_strIpInternal;
    USHORT m_uPortInternal;

    string m_strIpExternal;
    USHORT m_uPortExternal;

    ULONG m_uLastHeartbeat;

    ClientInfo()
    {
        m_uLastHeartbeat = 0;
        m_uPortInternal = 0;
        m_uPortExternal = 0;
    }
};

class CWorkLogic : public CDbgHandler
{
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    static void WINAPI OnRecv(const string &strData, const string &strAddr, USHORT uPort);
    void StartWork();
    void SendTo(const string &strData);
    vector<ClientInfo> GetClientList();

protected:
    bool SendData(CDbgClient *remote, const string &strData);
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**获取用户列表**/
    void RequestClients();
    void OnSingleData(CDbgClient *ptr, const string &strData);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnSendP2pHearbeat();
    void OnGetClients(const Value &vJson);
    bool SendForResult(CDbgClient *remote, Value &vRequest, string &strResult);
    virtual void onRecvData(CDbgClient *ptr, const string &strData);

protected:
    bool m_bInit;
    HANDLE m_hWorkThread;
    HANDLE m_hP2pNotify;
    string m_strLocalIp;
    USHORT m_uLocalPort;
    string m_strServIp;
    USHORT m_uServPort;

    CDbgClient m_c2serv;    //指向服务端
    CDbgClient m_c2client;  //指向调试对端

    /**p2p数据**/
    bool m_bP2pStat;
    string m_strP2pIp;
    USHORT m_uP2pPort;
    ClientInfo m_p2pInfo;
    HANDLE m_hCompleteNotify;

    map<string, ClientInfo> m_clientInfos;
    CCriticalSectionLockable m_requsetLock;     //请求锁

    struct RequestInfo
    {
        int m_id;
        string *m_pReply;
        HANDLE m_hNotify;
    };
    map<int, RequestInfo *> m_requestPool;      //请求池
    static CWorkLogic *ms_inst;
};
#endif