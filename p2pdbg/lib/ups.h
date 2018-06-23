/**
����udp�Ŀɿ�����ʵ�� lougd
2018/06/21
�ýӿ���Ҫʵ�����¹��ܣ�

1.ģ������tcp send-ack��ʽ��֤����������
2.��֤���ʱ��
3.����Զ��ְ�����֤���������Ч��
*/
#ifndef UPS_SAFE_H_H_
#define UPS_SAFE_H_H_
#include <WinSock2.h>
#include <Windows.h>
#include <vector>
#include <list>
#include <map>
#include <string>
#include "LockBase.h"

using namespace std;

#define MAGIC_NUMBER            0xf1f3              //���ʶ����
#define TIMESLICE_STAT          1000                //״̬���ʱ��Ƭ
#define PACKET_LIMIT            512                 //�����߼�����С����(����������MTU)
#define MAX_TESTCOUNT           3                   //��ೢ���ش�����
#define MAX_RECVTIME            3000                //���շ��������ȴ�ʱ��

//����ָ��
#define OPT_SEND_DATA           0x0001              //��ȫ���ݴ���ָ��
#define OPT_POST_DATA           0x0002              //�������ݴ���ָ��

#define OPT_REQUEST_ACK         0x0011              //���ݽ���Ӧ���
#define OPT_KEEPALIVE           0x0012              //���Ӱ����(����)

#define MARK_SEND               "send"              //���ݷ��ͱ�ʶ
#define MARK_RECV               "recv"              //���ݽ��ձ�ʶ

/**
ͨ������tcp���ͣ�ackӦ��ķ�ʽ��֤���ݵĿɿ�����
���ͷ�����ÿ��ʱ��Ƭ�ڼ���Ƿ��л�ִ�����û�н������ݵ��ش�
Ϊ��������Ƽ򵥣�m_uSerial�ֶν���OPT_SEND_DATAָ����Ч
UpsHeader�и����ֶ�ͨ�����������
*/
struct UpsHeader
{
    unsigned short m_uMagic;        //upsͷУ��λ
    unsigned short m_uOpt;          //����ָ��

    unsigned short m_uSerial;       //������ OPT_SEND:���η��͵ķ����� OPT_ACK:�յ���������
    unsigned short m_uSize;         //�߼�����С OPT_SEND:���η��͵ķ����С OPT_ACK:�յ���������

    UpsHeader() {
        m_uMagic = MAGIC_NUMBER;
    }
};

struct PacketSendStat
{
    DWORD m_dwLastSendCount;
    DWORD m_dwTestCount;

    PacketSendStat()
    {
        m_dwLastSendCount = 0;
        m_dwTestCount = 0;
    }
};

struct PacketSendDesc
{
    UpsHeader m_header;
    PacketSendStat m_stat;
    string m_strContent;
};

struct PackageSendCache
{
    string m_strUnique;                 //���ӱ�ʶ
    bool m_bNeedCheckStat;              //�Ƿ���Ҫ���״̬�����еİ������ͳɹ���û��Ҫ���������
    string m_strRemoteIp;               //Զ�˵ĵ�ַ
    USHORT m_uReomtePort;               //Զ�˵Ķ˿�
    vector<PacketSendDesc *> m_sendSet; //����״̬����

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

struct PacketRecvDesc
{
    PackageInterval m_interval;             //���շ�����к�
    string m_strContent;                    //���շ���ľ�������
    DWORD m_dwRecvTickCount;                //������յ�ʱ��
};

struct PackageRecvCache
{
    string m_strUnique;                     //���ӱ�ʶ
    int m_iFirstSerial;                     //��һ�����
    vector<PacketRecvDesc> m_recvDescSet;   //�������еķ������
    vector<string> m_CompleteSet;           //����ɵķ������
    int m_iSerialGrow;                      //�����������������1-65536ѭ��ʹ�õģ���������������ѭ�������

    PackageRecvCache()
    {
        m_iSerialGrow = 0;
        m_iFirstSerial = -1;
    }
};

class Ups : public CCriticalSectionLockable
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
    bool InsertRecvInterval(PacketRecvDesc desc, vector<PacketRecvDesc> &descSet);
    bool PushCache(const string &strUnique, const char *ip, USHORT uPort, PacketSendDesc *desc);
    bool SendToInternal(const string &strIp, USHORT uPort, const string &strData);
    vector<PacketSendDesc *> GetLogicSetFromRawData(const string &strData, int iOpt);
    string GetConnectUnique(const string &ip, unsigned short uPort, const string &strMark);
    bool ClearCacheByResp(string strUnique, UpsHeader header);
    bool OnCheckPacketSendStat();
    bool OnCheckPacketRecvStat();
    bool OnRecvUdpData(const char *addr, unsigned short uPort, const char *pData, int iLength);
    bool OnRecvComplete(PackageRecvCache &recvCache);
    bool OnRecvUpsData(const string &strUnique, UpsHeader *pHeader, const string &strData);
    bool OnRecvUpsAck(const string &strUnique, UpsHeader *pHeader);
    bool OnRecvPostData(const string &strUnique, const string &strData);
    UpsHeader *PacketHeader(unsigned short uOpt, unsigned short uSerial, unsigned short uLength, UpsHeader *ptr);
    UpsHeader *EncodeHeader(UpsHeader *pHeader);
    UpsHeader *DecodeHeader(UpsHeader *pHeader);
    unsigned short GetSendSerial();
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