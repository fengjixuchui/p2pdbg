#ifndef P2PSERV_LOGIC_H_H_
#define P2PSERV_LOGIC_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <list>
#include <string>
#include <map>
#include "../ComLib/json/json.h"
#include "../ComLib/LockBase.h"
#include "dbgserv.h"

using namespace std;
using namespace Json;

#define PORT_SERV           9692
#define TIMECOUNT_MAX       (1000 * 60)

struct FileTransInfo
{
    string m_strSrcUnique;          //�ļ�����Դ�˵�ַ
    string m_strDstUnique;          //�ļ�����Զ˵�ַ
    SOCKET m_sockSrc;               //�ļ�����Դ��socket
    SOCKET m_sockDst;               //�ļ�����Զ�socket
    ULONGLONG m_uFileSize;          //�ļ��ܴ�С
    ULONGLONG m_uRecvSize;          //�ļ��ѽ��մ�С
    DWORD m_dwLastRecvCount;        //�ϴη������ʱ��

    FileTransInfo()
    {
        m_dwLastRecvCount = 0;
        m_uFileSize = 0;
        m_uRecvSize = 0;
        m_sockDst = INVALID_SOCKET;
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

    string m_strStartTime;

    ULONG m_uLastHeartbeat;
    SOCKET m_MsgSock;               //��Ϣsocket
    string m_strMsgCache;           //��Ϣ���棨����ְ���

    SOCKET m_FtpSock;               //�ļ�socket
    string m_strFtpCache;           //�ļ����棨����ְ���

    int m_iTransMode;               //��������0����ͨ���� 1���ļ�������
    FileTransInfo m_fileInfo;       //���ڴ����ļ�����Ϣ

    ClientInfo()
    {
        m_iTransMode = 0;
        m_uLastHeartbeat = 0;
        m_uPortInternal = 0;
        m_uPortExternal = 0;
        m_MsgSock = INVALID_SOCKET;
        m_FtpSock = INVALID_SOCKET;
    }
};

enum FTP_STAT {
    em_ftp_init,        //��ʼ״̬
    em_ftp_start,       //��ʼ�ļ�����
    em_ftp_transing,    //�����ļ���
    em_ftp_succ,        //�����ļ��ɹ�
    em_ftp_faild        //�����ļ�ʧ��
};

struct FtpFile {
    string m_strSrc;
    string m_strDst;
    string m_strDesc;
    string m_strFileName;
    string m_strLocalPath;
    string m_strFtpUnique;
    int m_iFileSize;
    bool m_bCompress;
    int m_iNotifyCount;

    void reset() {
        m_strSrc.clear();
        m_strDst.clear();
        m_strDesc.clear();
        m_strFileName.clear();
        m_strLocalPath.clear();
        m_iFileSize = 0;
        m_strLocalPath.clear();
        m_bCompress = false;
        m_iNotifyCount = 0;
    }

    FtpFile() {
        m_iNotifyCount = 0;
        m_iFileSize = 0;
        m_bCompress = false;
    }
};

/**
ftp���ݻ��棬֮ǰ�Ŀ�ܺ���Ϣ��������Ϲ���
*/
struct FtpInfo {
    string m_strSrc;            //�ļ����ͷ�
    string m_strDst;            //�ļ����շ�
    string m_strDesc;           //�ļ�����
    string m_strFileName;       //�ļ���
    string m_strRecvCache;      //���ݽ��ջ���
    int m_iFileSize;            //�ļ��ܴ�С
    int m_iRecvSize;            //�ļ����մ�С
    string m_strLocalPath;      //�ļ����浽���ص�·��
    HANDLE m_hFileHandle;       //�����ļ����
    bool m_bCompress;           //�ļ��Ƿ�ѹ��
    SOCKET m_ftpSock;           //ftp����socket
    FTP_STAT m_eStat;           //����״̬
    DWORD m_dwLastCount;        //�����ʱ��

    void reset() {
        m_strSrc.clear();
        m_strDst.clear();
        m_strDesc.clear();
        m_strFileName.clear();
        m_strRecvCache.clear();
        m_iFileSize = 0;
        m_iRecvSize = 0;
        m_strLocalPath.clear();
        m_hFileHandle = INVALID_HANDLE_VALUE;
        m_bCompress = false;
        m_ftpSock = INVALID_SOCKET;
        m_eStat = em_ftp_init;
        m_dwLastCount = 0;
    }

    FtpInfo() {
        m_iFileSize = 0;
        m_iRecvSize = 0;
        m_hFileHandle = INVALID_HANDLE_VALUE;
        m_bCompress = false;
        m_ftpSock = INVALID_SOCKET;
        m_eStat = em_ftp_init;
        m_dwLastCount = 0;
    }
};

//��Ϣ����ӿ�
class DbgServMsgHandler :public DbgServHandler
{
    virtual bool OnSocketAccept(SOCKET sock);
    virtual bool OnDataRecv(SOCKET sock, const string &strRecv, string &strResp);
    virtual bool OnSocketClose(SOCKET sock);
};

//�ļ�����ӿ�
class DbgServFtpHandler :public DbgServHandler
{
    virtual bool OnSocketAccept(SOCKET sock);
    virtual bool OnDataRecv(SOCKET sock, const string &strRecv, string &strResp);
    virtual bool OnSocketClose(SOCKET sock);
};

class CWorkLogic
{
protected:
    CWorkLogic();

public:
    static CWorkLogic *GetInstance();
    bool StartWork(USHORT uMsgPort, USHORT uFtpPort);
    static void WINAPI OnRecv(const string &strData, const string &strAddr, USHORT uPort);

protected:
    bool SendData(SOCKET sock, const Value &vData);
    Value makeReply(int id, int iStat, const Value &vContent);
    void OnCheckClientStat();
    static DWORD WINAPI ServWorkThread(LPVOID pParam);
    static DWORD WINAPI KeepAliveThread(LPVOID pParam);
    static DWORD WINAPI FtpPushThread(LPVOID pParam);
    bool IsTcpPortAlive(USHORT uPort);
    string GetIpFromDomain(const string &strDomain);
    bool Connect(SOCKET sock, const string &strServIp, unsigned short uPort, int iTimeOut);

    //���ݻص�
protected:
    friend class DbgServMsgHandler;
    friend class DbgServFtpHandler;
    //��Ϣ�ص��ӿ�
    void OnGetClient(SOCKET sock, const Value &vJson);
    void OnClientHeartbeat(SOCKET sock, const Value &vJson);
    void OnTransData(SOCKET sock, const Value &vJson);
    void OnConnentClient(SOCKET sock, const Value &vJson);
    void OnMsgSingleData(SOCKET sock, const string &strData);
    ClientInfo *GetClientFromMsgSock(SOCKET sock);
    ClientInfo *GetClientFromUnique(const string &strUnique);
    virtual bool OnMsgSocketAccept(SOCKET sock);
    virtual bool OnMsgDataRecv(SOCKET sock, const string &strRecv, string &strResp);
    virtual bool OnMsgSocketClose(SOCKET sock);

    //�ļ��ص��ӿ�
    void OnFtpRegister(SOCKET sock, const Value &vJson);
    void CWorkLogic::OnFtpGetFile(SOCKET sock, const Value &vJson);
    //�ļ�����ӿ�
    void SendFile(SOCKET sock, const string &strFilePath);
    void OnPushFtpFile();
    FtpInfo *GetFtpInfoBySocket(SOCKET sock);
    void OnFtpTransferStat(FtpInfo *pFtpInfo, string &strData);
    void OnFtpTransferBegin(SOCKET sock, const Value &vJson);
    void OnFtpSingleData(SOCKET sock, const string &strData);
    ClientInfo *GetClientFromFtpSock(SOCKET sock);
    virtual bool OnFtpSocketAccept(SOCKET sock);
    virtual bool OnFtpDataRecv(SOCKET sock, const string &strRecv, string &strResp);
    virtual bool OnFtpSocketClose(SOCKET sock);
    string GetLoaclFtpPath(string strDstUnique, bool bCompress);

protected:
    DbgServMsgHandler m_MsgHandler;
    DbgServFtpHandler m_FtpHandler;
protected:
    CCriticalSectionLockable m_DataLock;

protected:
    bool m_bInit;
    list<ClientInfo *> m_clientSet;
    HANDLE m_hWorkThread;
    HANDLE m_hKeepAliveThread;
    CDbgServ m_dbgServ;
    static CWorkLogic *ms_inst;

    HANDLE m_hFtpNotify;
    HANDLE m_hFtpPushThread;
    list<FtpInfo *> m_ftpSet;
    list<FtpFile *> m_fileSet;
    CDbgServ m_ftpServ;
};
#endif