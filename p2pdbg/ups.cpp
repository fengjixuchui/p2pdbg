#include <WinSock2.h>
#include <gdcharconv.h>
#include "ups.h"

Ups::Ups()
{
    m_bInit = false;
    m_uLocalPort = 0;
    m_iSendSerial = 0;
    m_udpSocket = INVALID_SOCKET;
    m_hRecvThread = NULL;
    m_hStatThread = NULL;
}

Ups::~Ups()
{}

string Ups::GetConnectUnique(const string &ip, unsigned short uPort, const string &strMark)
{
    return fmt("%hs_%d-%hs_%d_%hs", ip.c_str(), uPort, m_strLocalIp.c_str(), m_uLocalPort, strMark.c_str());
}

bool Ups::InsertRecvInterval(PackageInterval interval, vector<PackageInterval> &intervalSet)
{
    vector<PackageInterval>::const_iterator it;
    for (it = intervalSet.begin() ; it != intervalSet.end() ; it++)
    {
        if (interval.m_iStartSerial < it->m_iStartSerial)
        {
            intervalSet.insert(it, interval);
        }
        //封包发送重复
        else if (interval.m_iStartSerial == it->m_iStartSerial)
        {
            return false;
        }
    }

    if (it == intervalSet.end())
    {
        intervalSet.push_back(interval);
    }
    return true;
}

bool Ups::SendAck(const char *ip, USHORT uPort, UpsHeader *pHeader)
{
    UpsHeader ack;
    ack.m_uOpt = OPT_REQUEST_ACK;
    ack.m_uSerial = pHeader->m_uSerial;
    ack.m_uSize = pHeader->m_uSize;
    return SendToInternal(ip, uPort, string((const char *)&ack, sizeof(ack)));
}

bool Ups::OnRecvComplete(PackageRecvCache &recvCache)
{
    int iCount = 0;
    vector<PackageInterval> interval = recvCache.m_intervalSet;
    for (vector<PackageInterval>::iterator ij = interval.begin() ; ij != interval.end() ; ij++, iCount++)
    {
        if (ij->m_iStartSerial == recvCache.m_iFirstSerial)
        {
            recvCache.m_iFirstSerial += ij->m_iPackageSize;
            recvCache.m_strCompleteBuffer += recvCache.m_packageSet[iCount];
        }
        else
        {
            break;
        }
    }

    if (iCount > 0)
    {
        recvCache.m_intervalSet.erase(recvCache.m_intervalSet.begin(), recvCache.m_intervalSet.begin() + iCount);
        recvCache.m_packageSet.erase(recvCache.m_packageSet.begin(), recvCache.m_packageSet.begin() + iCount);
    }
    return true;
}

bool Ups::OnRecvUpsData(const string &strUnique, UpsHeader *pHeader, const string &strData)
{
    map<string, PackageRecvCache>::iterator it;
    if (m_recvCache.end() == (it = m_recvCache.find(strUnique)))
    {
        PackageRecvCache cache;
        cache.m_iFirstSerial = pHeader->m_uSerial + pHeader->m_uSize;
        cache.m_strCompleteBuffer += strData;
    }
    else
    {
        if (pHeader->m_uSerial == it->second.m_iFirstSerial)
        {
            it->second.m_iFirstSerial += pHeader->m_uSize;
            it->second.m_strCompleteBuffer += strData;
            vector<PackageInterval> interval = it->second.m_intervalSet;
            OnRecvComplete(it->second);
        }
        else
        {
            PackageInterval interval;
            interval.m_iStartSerial = pHeader->m_uSerial;
            interval.m_iPackageSize = pHeader->m_uSize;
            //将封包区间插入区间集合，等待之前的封包接收
            InsertRecvInterval(interval, it->second.m_intervalSet);
        }
    }
    return true;
}

bool Ups::OnRecvUpsAck(const string &strUnique, UpsHeader *pHeader)
{
    map<string, PackageSendCache>::iterator it = m_sendCache.find(strUnique);
    if (it == m_sendCache.end())
    {
        return false;
    }

    for (vector<PacketSendDesc *>::iterator ij = it->second.m_sendSet.begin() ; ij != it->second.m_sendSet.end() ;)
    {
        PacketSendDesc *pDesc = (*ij);
        if (pHeader->m_uSerial == pDesc->m_header.m_uSerial)
        {
            ij = it->second.m_sendSet.erase(ij);
            delete pDesc;
            break;
        }
        else
        {
            ij++;
        }
    }
    return true;
}

bool Ups::OnRecvPostData(const string &strUnique, const string &strData)
{
    m_recvCache[strUnique].m_strCompleteBuffer += strData;
    return true;
}

bool Ups::OnRecvUdpData(const char *addr, unsigned short uPort, const char *pData, int iLength)
{
    UpsHeader *ptr = (UpsHeader *)pData;
    if (ptr->m_uMagic != MAGIC_NUMBER)
    {
        return false;
    }

    string strUnique;
    int iDataLength = iLength - sizeof(UpsHeader);
    switch (ptr->m_uOpt) {
        case OPT_SEND_DATA:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_RECV);
                SendAck(addr, uPort, ptr);
                OnRecvUpsData(strUnique, ptr, string(pData + sizeof(UpsHeader), iDataLength));
            }
            break;
        case OPT_POST_DATA:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_RECV);
                OnRecvPostData(strUnique, string(pData + sizeof(UpsHeader), iDataLength));
            }
            break;
        case OPT_REQUEST_ACK:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_SEND);
                OnRecvUpsAck(strUnique, ptr);
            }
            break;
        case OPT_KEEPALIVE:
            break;
        default:
            break;
    }
    return true;
}

DWORD Ups::RecvThread(LPVOID pParam)
{
    Ups *ptr = (Ups *)pParam;
    char buffer[4096];
    int iBufferSize = sizeof(buffer);
    SOCKADDR_IN clientAddr = {0};
    int iAddrSize = sizeof(clientAddr);

    while (true)
    {
        iAddrSize = sizeof(clientAddr);
        int iRecvSize = recvfrom(ptr->m_udpSocket, buffer, iBufferSize, 0, (sockaddr *)&clientAddr, &iAddrSize);

        if (iRecvSize <= 0)
        {
            break;
        }

        if(iRecvSize < sizeof(UpsHeader))
        {
            continue;
        }

        string strAddr = inet_ntoa(clientAddr.sin_addr);
        USHORT uPort = ntohs(clientAddr.sin_port);
        ptr->OnRecvUdpData(strAddr.c_str(), uPort, buffer, iRecvSize);
    }
    return 0;
}

bool Ups::SendToInternal(const string &strIp, USHORT uPort, const string &strData)
{
    SOCKADDR_IN addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = inet_addr(strIp.c_str());
    addr.sin_port = htons(uPort);
    sendto(m_udpSocket, strData.c_str(), strData.length(), 0, (const sockaddr *)&addr, sizeof(addr));
    return true;
}

bool Ups::OnCheckPacketSendStat()
{
    DWORD dwCurCount = GetTickCount();
    for (map<string, PackageSendCache>::const_iterator it = m_sendCache.begin() ; it != m_sendCache.end() ; it++)
    {
        PackageSendCache cache = it->second;
        if (!cache.m_bNeedCheckStat)
        {
            continue;
        }

        for (vector<PacketSendDesc *>::iterator ij = cache.m_sendSet.begin() ; ij != cache.m_sendSet.end() ; ij++)
        {
            PacketSendDesc *desc = *ij;
            if (dwCurCount - desc->m_stat.m_dwLastSendCount >= TIMESLICE_STAT)
            {
                desc->m_stat.m_dwLastSendCount = dwCurCount;
                SendToInternal(cache.m_strRemoteIp, cache.m_uReomtePort, desc->m_strContent);
            }
        }
    }
    return true;
}

bool Ups::OnCheckPacketRecvStat()
{
    DWORD dwCurCount = GetTickCount();
    for (map<string, PackageRecvCache>::iterator it = m_recvCache.begin() ; it != m_recvCache.end() ; it++)
    {
        if (it->second.m_packageSet.size() > 0 && ((dwCurCount - it->second.m_dwLastUpdateCount) > MAX_RECVTIME))
        {
            it->second.m_iFirstSerial = it->second.m_intervalSet.begin()->m_iStartSerial;
            OnRecvComplete(it->second);
        }
    }
    return true;
}

vector<PacketSendDesc *> Ups::GetLogicSetFromRawData(const string &strData, int opt)
{
    int iPos = 0;
    vector<PacketSendDesc *> result;
    int iRealSize = (PACKET_LIMIT - sizeof(UpsHeader));
    int iFreeSize = strData.size();
    while (true)
    {
        if (iPos >= (int)strData.size())
        {
            break;
        }

        if (iFreeSize <= 0)
        {
            break;
        }

        if (iFreeSize <= iRealSize)
        {
            break;
        }
        PacketSendDesc *ptr = new PacketSendDesc();
        ptr->m_header.m_uMagic = MAGIC_NUMBER;
        ptr->m_header.m_uOpt = opt;

        if (opt == OPT_SEND_DATA)
        {
            ptr->m_header.m_uSerial = m_iSendSerial++;
        }
        else
        {
            ptr->m_header.m_uSerial = 0;
        }
        ptr->m_header.m_uSize = iRealSize;
        ptr->m_strContent.append((const char *)(&(ptr->m_header)), sizeof(UpsHeader));
        ptr->m_strContent += strData.substr(iPos, iRealSize);
        result.push_back(ptr);

        iPos += iRealSize;
        iFreeSize -= iRealSize;
    }

    if (iPos < (int)strData.size())
    {
        PacketSendDesc *ptr = new PacketSendDesc();
        ptr->m_header.m_uMagic = MAGIC_NUMBER;
        ptr->m_header.m_uOpt = OPT_SEND_DATA;
        ptr->m_header.m_uSerial = m_iSendSerial++;
        ptr->m_header.m_uSize = strData.size() - iPos;

        ptr->m_strContent.append((const char *)(&(ptr->m_header)), sizeof(UpsHeader));
        ptr->m_strContent += strData.substr(iPos, strData.size() - iPos);
        result.push_back(ptr);
    }
    return result;
}

DWORD Ups::StatThread(LPVOID pParam)
{
    Ups *ptr = (Ups *)pParam;
    HANDLE arry [] = {ptr->m_hStatEvent, ptr->m_hStopEvent};
    while (true)
    {
        DWORD dwRet = WaitForMultipleObjects(RTL_NUMBER_OF(arry), arry, FALSE, TIMESLICE_STAT);

        if ((WAIT_OBJECT_0 + 1) == dwRet)
        {
            break;
        }
        ptr->OnCheckPacketSendStat();
        ptr->OnCheckPacketRecvStat();
    }
    return 0;
}

bool Ups::UpsInit(unsigned short uLocalPort, bool bKeepAlive)
{
    if (m_bInit)
    {
        return false;
    }

    m_bInit = true;
    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_uLocalPort = uLocalPort;

    SOCKADDR_IN localAddr = {0};
    localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(uLocalPort);
    bind(m_udpSocket, (sockaddr *)&localAddr, sizeof(localAddr));

    m_hStatEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    m_hRecvThread = CreateThread(NULL, 0, RecvThread, this, 0, NULL);
    m_hStatThread = CreateThread(NULL, 0, StatThread, this, 0, NULL);
    return true;
}

bool Ups::UpsClose()
{
    if (!m_bInit)
    {
        return false;
    }

    m_bInit = false;
    closesocket(m_udpSocket);
    m_udpSocket = INVALID_SOCKET;
    SetEvent(m_hStopEvent);
    m_hStopEvent = NULL;
    return true;
}

bool Ups::UpsPost(const char *addr, unsigned short uPort, const char *pData, int iLength)
{
    vector<PacketSendDesc *> result = GetLogicSetFromRawData(string(pData, iLength), OPT_POST_DATA);
    for (vector<PacketSendDesc *>::iterator it = result.begin() ; it != result.end() ; it++)
    {
        PacketSendDesc *ptr = *it;
        SendToInternal(addr, uPort, ptr->m_strContent);
    }
    return true;
}

bool Ups::PushCache(const string &strUnique, const char *ip, USHORT uPort, PacketSendDesc *desc)
{
    map<string, PackageSendCache>::iterator it;
    desc->m_stat.m_dwLastSendCount = GetTickCount();
    if (m_sendCache.end() == (it = m_sendCache.find(strUnique)))
    {
        PackageSendCache cache;
        cache.m_bNeedCheckStat = true;
        cache.m_strRemoteIp = ip;
        cache.m_uReomtePort = uPort;
        cache.m_strUnique = strUnique;
        cache.m_sendSet.push_back(desc);
        m_sendCache[strUnique] = cache;
    }
    else
    {
        it->second.m_bNeedCheckStat = true;
        it->second.m_sendSet.push_back(desc);
    }
    return true;
}

bool Ups::UpsSend(const char *addr, unsigned short uPort, const char *pData, int iLength)
{
    vector<PacketSendDesc *> result = GetLogicSetFromRawData(string(pData, iLength), OPT_SEND_DATA);
    string strUnique = GetConnectUnique(addr, uPort, MARK_SEND);
    for (vector<PacketSendDesc *>::iterator it = result.begin() ; it != result.end() ; it++)
    {
        PacketSendDesc *ptr = *it;
        PushCache(strUnique, addr, uPort, ptr);
        SendToInternal(addr, uPort, ptr->m_strContent);
        ptr->m_stat.m_dwLastSendCount = GetTickCount();
    }
    return true;
}

int Ups::UpsRecv(string &strIp, USHORT &uPort, string &strData)
{
    for (map<string, PackageRecvCache>::iterator it = m_recvCache.begin() ; it != m_recvCache.end() ; it++)
    {
        if (!it->second.m_strCompleteBuffer.empty())
        {
            strData = it->second.m_strCompleteBuffer;
            int iSize = it->second.m_strCompleteBuffer.size();
            it->second.m_strCompleteBuffer.clear();
            return iSize;
        }
    }
    return 0;
}