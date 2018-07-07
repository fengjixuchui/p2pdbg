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
    string m_strFileDesc;       //�ļ�����
    string m_strFileName;       //�ļ���

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

class CWorkLogic : public CDbgHandler
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
    bool SendToDbgClient(Value &vData, string &strReply, int iTimeOut = -1);
    bool SendData(CDbgClient *remote, const string &strData);
    void GetJsonPack(Value &v, const string &strDataType);
    void SetServAddr(const string &strServIp, USHORT uServPort);
    string GetDevDesc();
    /**��ȡ�û��б�**/
    string RequestClientInternal();
    void OnSingleData(CDbgClient *ptr, const string &strData);
    static DWORD WINAPI WorkThread(LPVOID pParam);
    void OnSendServHeartbeat();
    void OnGetClientsInThread();
    void OnReply(CDbgClient *ptr, const string &strData);
    void OnTransData(CDbgClient *ptr, const string &strData);
    bool SendForResult(CDbgClient *remote, int id, Value &vRequest, string &strResult, int iTimeOut = -1);
    virtual void onRecvData(CDbgClient *ptr, const string &strData);

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

    CDbgClient m_c2serv;    //ָ������
    CDbgClient m_c2client;  //ָ����ԶԶ�

    HANDLE m_hCompleteNotify;
    ClientInfo m_DbgClient;
    map<string, ClientInfo> m_clientInfos;
    CCriticalSectionLockable m_requsetLock;     //������
    CCriticalSectionLockable m_clientLock;      //�ͻ�����Ϣ��

    struct RequestInfo
    {
        int m_id;
        string *m_pReply;
        HANDLE m_hNotify;
    };
    map<int, RequestInfo *> m_requestPool;      //�����
    static CWorkLogic *ms_inst;
};
#endif