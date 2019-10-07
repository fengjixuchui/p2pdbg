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
    string m_strSrcUnique;          //文件传输源端地址
    string m_strDstUnique;          //文件传输对端地址
    SOCKET m_sockSrc;               //文件传输源端socket
    SOCKET m_sockDst;               //文件传输对端socket
    ULONGLONG m_uFileSize;          //文件总大小
    ULONGLONG m_uRecvSize;          //文件已接收大小
    DWORD m_dwLastRecvCount;        //上次封包接收时间

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
    SOCKET m_MsgSock;               //消息socket
    string m_strMsgCache;           //消息缓存（处理分包）

    SOCKET m_FtpSock;               //文件socket
    string m_strFtpCache;           //文件缓存（处理分包）

    int m_iTransMode;               //传输类型0：普通传输 1：文件流传输
    FileTransInfo m_fileInfo;       //正在传输文件的信息

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
    em_ftp_init,        //初始状态
    em_ftp_start,       //开始文件传输
    em_ftp_transing,    //传输文件中
    em_ftp_succ,        //传输文件成功
    em_ftp_faild        //传输文件失败
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
ftp数据缓存，之前的框架和消息服务器耦合过高
*/
struct FtpInfo {
    string m_strSrc;            //文件发送方
    string m_strDst;            //文件接收方
    string m_strDesc;           //文件描述
    string m_strFileName;       //文件名
    string m_strRecvCache;      //数据接收缓存
    int m_iFileSize;            //文件总大小
    int m_iRecvSize;            //文件接收大小
    string m_strLocalPath;      //文件缓存到本地的路径
    HANDLE m_hFileHandle;       //本地文件句柄
    bool m_bCompress;           //文件是否被压缩
    SOCKET m_ftpSock;           //ftp传输socket
    FTP_STAT m_eStat;           //传输状态
    DWORD m_dwLastCount;        //最后传输时间

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

//消息处理接口
class DbgServMsgHandler :public DbgServHandler
{
    virtual bool OnSocketAccept(SOCKET sock);
    virtual bool OnDataRecv(SOCKET sock, const string &strRecv, string &strResp);
    virtual bool OnSocketClose(SOCKET sock);
};

//文件处理接口
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

    //数据回调
protected:
    friend class DbgServMsgHandler;
    friend class DbgServFtpHandler;
    //消息回调接口
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

    //文件回调接口
    void OnFtpRegister(SOCKET sock, const Value &vJson);
    void CWorkLogic::OnFtpGetFile(SOCKET sock, const Value &vJson);
    //文件传输接口
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