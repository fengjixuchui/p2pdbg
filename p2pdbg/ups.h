/**
基于udp的可靠传输实现 lougd
2018/06/21
*/
#ifndef UPS_SAFE_H_H_
#define UPS_SAFE_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <map>
#include <string>

using namespace std;

#define MAGIC_NUMBER            0xf1f3
#define TIMESLICE_STAT          1000
#define PACKET_LIMIT            512

//操作指令
#define OPT_SEND_DATA           0x0001
#define OPT_POST_DATA           0x0002

#define OPT_REQUEST_MISS        0x0010
#define OPT_REQUEST_ACK         0x0011
#define OPT_KEEPALIVE           0x0012

#define MARK_SEND               "send"
#define MARK_RECV               "recv"

/**
通过类似tcp发送，ack应答的方式保证数据的可靠发送
发送方会在每个时间片内检测是否有回执，如果没有进行数据的重传
为尽可能设计简单，m_uSerial字段仅对OPT_SEND_DATA指令有效
*/
struct UpsHeader
{
    unsigned short m_uMagic;        //ups头校验位
    unsigned short m_uOpt;          //操作指令

    unsigned short m_uSerial;   //封包序号 OPT_SEND:本次发送的封包序号 OPT_ACK:收到封包的序号
    unsigned short m_uSize;     //逻辑包大小 OPT_SEND:本次发送的封包大小 OPT_ACK:收到封包的序号

    UpsHeader() {
        m_uMagic = MAGIC_NUMBER;
    }
};

struct PacketSendStat
{
    DWORD m_dwLastSendCount;
};

struct PacketSendDesc
{
    UpsHeader m_header;
    PacketSendStat m_stat;
    string m_strContent;
};

struct PackageSendCache
{
    string m_strUnique;                 //连接标识
    bool m_bNeedCheckStat;              //是否需要检查状态，所有的包都发送成功就没必要继续检查了
    string m_strRemoteIp;               //远端的地址
    USHORT m_uReomtePort;               //远端的端口
    vector<PacketSendDesc *> m_sendSet; //发送状态缓存

    PackageSendCache()
    {
        m_bNeedCheckStat = false;
        m_uReomtePort = 0;
    }
};

struct PackageInterval
{
    int m_iStartSerial;
    int m_iPackageSize;
};

struct PackageRecvCache
{
    string m_strUnique;                     //连接标识
    int m_iFirstSerial;                     //第一个序号
    vector<PackageInterval> m_intervalSet;  //区间集合
    vector<string> m_packageSet;            //封包内容缓存集合
    string m_strCompleteBuffer;             //已完成的封包集合
};

class Ups
{
public:
    Ups();
    virtual ~Ups();

    bool UpsInit(unsigned short uLocalPort, bool bKeepAlive);
    bool UpsClose();
    bool UpsPost(const char *addr, unsigned short uPort, const char *pData, int iLength);
    bool UpsSend(const char *addr, unsigned short uPort, const char *pData, int iLength);
    int UpsRecv(string &strIp, USHORT &uPort, string &strData);

protected:
    bool SendAck(const char *ip, USHORT uPort, UpsHeader *pHeader);
    bool InsertRecvInterval(PackageInterval interval, vector<PackageInterval> &intervalSet);
    bool PushCache(const string &strUnique, const char *ip, USHORT uPort, PacketSendDesc *desc);
    bool SendToInternal(const string &strIp, USHORT uPort, const string &strData);
    vector<PacketSendDesc *> GetLogicSetFromRawData(const string &strData, int iOpt);
    string GetConnectUnique(const string &ip, unsigned short uPort, const string &strMark);
    bool ClearCacheByResp(string strUnique, UpsHeader header);
    bool OnCheckPacketSendStat();
    bool OnRecvUdpData(const char *addr, unsigned short uPort, const char *pData, int iLength);
    bool OnRecvUpsData(const string &strUnique, UpsHeader *pHeader, const string &strData);
    bool OnRecvUpsAck(const string &strUnique, UpsHeader *pHeader);
    static DWORD WINAPI RecvThread(LPVOID pParam);
    static DWORD WINAPI StatThread(LPVOID pParam);

protected:
    bool m_bInit;
    int m_uLocalPort;
    string m_strLocalIp;
    int m_iSendSerial;
    SOCKET m_udpSocket;
    HANDLE m_hRecvThread;
    HANDLE m_hStatThread;
    HANDLE m_hStatEvent;
    HANDLE m_hStopEvent;

    map<string, PackageSendCache> m_sendCache;
    map<string, PackageRecvCache> m_recvCache;
};
#endif