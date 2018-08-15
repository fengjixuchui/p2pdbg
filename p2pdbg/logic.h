#ifndef P2PDBG_LOGIC_H_H_
#define P2PDBG_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <mstring.h>
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
 �ļ�����
 {
     "dataType":"fileTransfer",
     "id":112233,
     "fileUnique":"�ļ���ʶ",
     "src":"�ļ����ͷ�",
     "dst":"�ļ����շ�",
     "desc":"�ļ�����",
     "fileSize":112233,
     "fileName":"�ļ���"
 }
 <File Start>
 ������ļ�����
*/
struct FileTransInfo
{
    ULONGLONG m_uFileSize;      //�ļ��ܴ�С
    ULONGLONG m_uRecvSize;      //�ļ��ѽ��մ�С
    ustring m_wstrFileDesc;     //�ļ�����
    ustring m_wstrFileName;     //�ļ���
    ustring m_wstrLocalPath;    //�ļ�����·��
    HANDLE m_hTransferFile;     //��־�ļ����
    bool m_bCompress;           //�Ƿ���ѹ���ļ�

    FileTransInfo()
    {
        m_bCompress = true;
        m_uFileSize = 0;
        m_uRecvSize = 0;
        m_hTransferFile = INVALID_HANDLE_VALUE;
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
    bool StartWork();
    vector<ClientInfo> GetClientList();
    bool ConnectReomte(const string &strRemote);
    bool IsConnectDbg();
    ClientInfo GetDbgClient();
    string GetDbgUnique();
    wstring ExecCmd(const wstring &wstrCmd, int iTimeOut = -1);

protected:
    bool RegisterMsgServSyn();
    bool RegisterFtpServSyn();
    mstring GetDevUnique();
    string GetIpFromDomain(const string &strDomain);
    bool SendToDbgClient(Value &vData, string &strReply, int iTimeOut = -1);
    bool SendData(CDbgClient *remote, const Value &vData);
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**��ȡ�û��б�**/
    string RequestClientInternal();
    void OnMsgSingleData(const string &strData);
    void OnFtpSingleData(const string &strData);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnGetClientsInThread();
    void OnMsgReply(const string &strData);
    void OnMsgTransData(const string &strData);

    bool SendFtpForResult(int id, Value &vRequest, string &strResult, DWORD iTimeOut = -1);
    bool SendMsgForResult(int id, Value &vRequest, string &strResult, DWORD iTimeOut = -1);

    void OnRecvMsgData(const string &strData);

    void OnFtpTransferStat(string &strData);
    void OnFtpReply(const string &strData);
    void OnRecvFtpData(const string &strData);
    wstring GetFtpLocalPath(const ustring &wstrDesc, const ustring &wstrUnique, const ustring &wstrFileName);
    void OnFileTransferBegin(Value &vJson);

protected:
    bool m_bInit;
    bool m_bConnectSucc;    //�Ƿ����ӳɹ�
    int m_iDbgMode;         //����ģʽ 0:c2c 1:c2s

    HANDLE m_hWorkThread;
    string m_strLocalIp;
    USHORT m_uLocalPort;
    string m_strServIp;
    string m_strDevUnique;
    USHORT m_uServPort;

    CDbgClient m_MsgServ;    //ָ������
    CDbgClient m_FtpServ;    //ָ����ԶԶ�

    friend class CDbgMsgHanler;
    friend class CDbgFtpHandler;
    CDbgMsgHanler m_MsgHandler;
    CDbgFtpHandler m_FtpHandler;

    HANDLE m_hCompleteNotify;
    ClientInfo m_DbgClient;
    map<string, ClientInfo> m_clientInfos;
    CCriticalSectionLockable m_requsetLock;     //������
    CCriticalSectionLockable m_clientLock;      //�ͻ�����Ϣ��

    //�ļ����仺��
    bool m_bFtpTransfer;
    FileTransInfo m_FtpCache;

    struct RequestInfo
    {
        int m_id;
        string *m_pReply;
        HANDLE m_hNotify;
    };
    map<int, RequestInfo *> m_MsgRequestPool;      //��Ϣ�����������
    string m_strMsgCache;                          //��Ϣ�����������
    map<int, RequestInfo *> m_FtpRequestPool;      //FTP�����������
    string m_strFtpCache;                          //FTP�����������
    static CWorkLogic *ms_inst;
};
#endif