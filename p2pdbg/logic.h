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
    void StartWork();
    vector<ClientInfo> GetClientList();
    bool ConnectReomte(const string &strRemote);

protected:
    bool SendToDbgClient(const string &strData);
    bool SendData(CDbgClient *remote, const string &strData);
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**获取用户列表**/
    string RequestClientInternal();
    void OnSingleData(CDbgClient *ptr, const string &strData);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnGetClientsInThread();
    void OnReply(CDbgClient *ptr, const string &strData);
    void OnTransData(CDbgClient *ptr, const string &strData);
    bool SendForResult(CDbgClient *remote, Value &vRequest, string &strResult);
    virtual void onRecvData(CDbgClient *ptr, const string &strData);

protected:
    bool m_bInit;

    bool m_bConnectSucc;    //是否连接成功
    int m_iDbgMode;         //调试模式 0:c2c 1:c2s

    HANDLE m_hWorkThread;
    string m_strLocalIp;
    USHORT m_uLocalPort;
    string m_strServIp;
    USHORT m_uServPort;

    CDbgClient m_c2serv;    //指向服务端
    CDbgClient m_c2client;  //指向调试对端

    HANDLE m_hCompleteNotify;
    ClientInfo m_DbgClient;
    map<string, ClientInfo> m_clientInfos;
    CCriticalSectionLockable m_requsetLock;     //请求锁
    CCriticalSectionLockable m_clientLock;      //客户端信息锁

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