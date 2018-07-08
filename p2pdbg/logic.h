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

#define DOMAIN_SERV         "p2pdbg.picp.io"
#define PORT_MSG_SERV       9971
#define PORT_FTP_SERV       9981
#define PORT_LOCAL          8807
#define DBGDESC             "p2pdbg"

/**
 文件传输
 {
     "dataType":"fileTransfer",
     "id":112233,
     "fileUnique":"文件标识",
     "src":"文件发送方",
     "dst":"文件接收方",
     "desc":"文件描述",
     "fileSize":112233,
     "fileName":"文件名"
 }
 <File Start>
 具体的文件内容
*/
struct FileTransInfo
{
    ULONGLONG m_uFileSize;      //文件总大小
    ULONGLONG m_uRecvSize;      //文件已接收大小
    string m_strFileDesc;       //文件描述
    string m_strFileName;       //文件名

    FileTransInfo()
    {
        m_uFileSize = 0;
        m_uRecvSize = 0;
    }
};

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

class CDbgMsgHanler : public CDbgHandler
{
    virtual void onRecvData(const string &strData);
};

class CDbgFtpHandler : public CDbgHandler
{
    virtual void onRecvData(const string &strData);
};

class CWorkLogic
{
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    void StartWork();
    vector<ClientInfo> GetClientList();
    bool ConnectReomte(const string &strRemote);
    bool IsConnectDbg();
    ClientInfo GetDbgClient();
    string GetDbgUnique();
    wstring ExecCmd(const wstring &wstrCmd, int iTimeOut = -1);

protected:
    bool RegisterMsgServSyn();
    bool RegisterFtpServSyn();
    string GetIpFromDomain(const string &strDomain);
    bool SendToDbgClient(Value &vData, string &strReply, int iTimeOut = -1);
    bool SendData(CDbgClient *remote, const Value &vData);
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**获取用户列表**/
    string RequestClientInternal();
    void OnMsgSingleData(const string &strData);
    void OnFtpSingleData(const string &strData);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnGetClientsInThread();
    void OnMsgReply(const string &strData);
    void OnMsgTransData(const string &strData);

    void OnFtpReply(const string &strData);

    bool SendFtpForResult(int id, Value &vRequest, string &strResult);
    bool SendMsgForResult(int id, Value &vRequest, string &strResult, int iTimeOut = -1);

    void OnRecvMsgData(const string &strData);
    void OnRecvFtpData(const string &strData);

protected:
    bool m_bInit;
    bool m_bConnectSucc;    //是否连接成功
    int m_iDbgMode;         //调试模式 0:c2c 1:c2s

    HANDLE m_hWorkThread;
    string m_strLocalIp;
    USHORT m_uLocalPort;
    string m_strServIp;
    string m_strDevUnique;
    USHORT m_uServPort;

    CDbgClient m_MsgServ;    //指向服务端
    CDbgClient m_FtpServ;    //指向调试对端

    friend class CDbgMsgHanler;
    friend class CDbgFtpHandler;
    CDbgMsgHanler m_MsgHandler;
    CDbgFtpHandler m_FtpHandler;

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
    map<int, RequestInfo *> m_MsgRequestPool;      //消息服务器请求池
    string m_strMsgCache;                          //消息服务器缓冲池
    map<int, RequestInfo *> m_FtpRequestPool;      //FTP服务器请求池
    string m_strFtpCache;                          //FTP服务器缓冲池
    static CWorkLogic *ms_inst;
};
#endif