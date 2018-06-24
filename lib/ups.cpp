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
    m_iRecvPacketCount = 0;
    m_uMagicNum = 0;
}

Ups::~Ups()
{}

string Ups::GetConnectUnique(const string &ip, unsigned short uPort, const string &strMark)
{
    return fmt("%hs_%d-%hs_%d_%hs", ip.c_str(), uPort, m_strLocalIp.c_str(), m_uLocalPort, strMark.c_str());
}

bool Ups::InsertRecvInterval(PacketRecvDesc desc, vector<PacketRecvDesc> &descSet)
{
    vector<PacketRecvDesc>::const_iterator it;
    for (it = descSet.begin() ; it != descSet.end() ; it++)
    {
        if (desc.m_interval.m_iStartSerial < it->m_interval.m_iStartSerial)
        {
            descSet.insert(it, desc);
            return true;
        }
        //封包发送重复
        else if (desc.m_interval.m_iStartSerial == it->m_interval.m_iStartSerial)
        {
            return false;
        }
    }

    if (it == descSet.end())
    {
        descSet.push_back(desc);
    }
    return true;
}

unsigned short Ups::GetMagicNumber()
{
    DWORD dw = GetTickCount();
    unsigned short hight = (dw & 0xff);
    unsigned short low = (hight ^ MAGIC_SEED);
    unsigned short uMagic = ((hight << 4) | low);
    return uMagic;
}

bool Ups::IsValidMagic(unsigned short uMagic)
{
    unsigned short hight = (uMagic >> 4);
    unsigned short low = (uMagic & 0xff);
    return (low == (hight ^ MAGIC_SEED));
}

bool Ups::SendAck(const char *ip, USHORT uPort, UpsHeader *pHeader)
{
    UpsHeader ack;
    PacketHeader(OPT_REQUEST_ACK, pHeader->m_uSerial, pHeader->m_uSize, &ack);
    return SendToInternal(ip, uPort, string((const char *)&ack, sizeof(ack)));
}

bool Ups::OnRecvComplete(PackageRecvCache &recvCache)
{
    int iCount = 0;
    vector<PacketRecvDesc> interval = recvCache.m_recvDescSet;
    for (vector<PacketRecvDesc>::iterator ij = interval.begin() ; ij != interval.end() ; ij++, iCount++)
    {
        if (ij->m_interval.m_iStartSerial == recvCache.m_iFirstSerial)
        {
            recvCache.m_iFirstSerial++;
            PushCompletePacket(recvCache, ij->m_strContent);
        }
        else
        {
            break;
        }
    }

    if (iCount > 0)
    {
        recvCache.m_recvDescSet.erase(recvCache.m_recvDescSet.begin(), recvCache.m_recvDescSet.begin() + iCount);
    }
    return true;
}

bool Ups::CheckDataMagic(UpsHeader *header, PackageRecvCache &cache)
{
    CScopedLocker lock(this);
    if (header->m_uMagic != cache.m_iMagicNum)
    {
        for (vector<PacketRecvDesc>::iterator it = cache.m_recvDescSet.begin() ; it != cache.m_recvDescSet.end() ; it++)
        {
            PushCompletePacket(cache, it->m_strContent);
        }
        cache.m_recvDescSet.clear();
        cache.m_iFirstSerial = header->m_uSerial;
        cache.m_iMagicNum = header->m_uMagic;
    }
    return true;
}

bool Ups::PushCompletePacket(PackageRecvCache &cache, const string &strData)
{
    cache.m_CompleteSet.push_back(strData);
    m_iRecvPacketCount++;

    string dbg = fmt("m_iRecvPacketCount:%d\n", m_iRecvPacketCount);
    //OutputDebugStringA(dbg.c_str());
    SetEvent(m_hRecvEvent);
    return true;
}

bool Ups::OnRecvUpsData(const char *addr, unsigned short uPort, const string &strUnique, UpsHeader *pHeader, const string &strData)
{
    CScopedLocker lock(this);
    map<string, PackageRecvCache>::iterator it;
    if (m_recvCache.end() == (it = m_recvCache.find(strUnique)))
    {
        PackageRecvCache cache;
        cache.m_strUnique = strUnique;
        cache.m_strIp = addr;
        cache.m_uPort = uPort;
        cache.m_iFirstSerial = pHeader->m_uSerial;
        cache.m_CompleteSet;
        cache.m_iMagicNum = pHeader->m_uMagic;
        m_recvCache[strUnique] = cache;
        PushCompletePacket(m_recvCache[strUnique], strData);
    }
    else
    {
        CheckDataMagic(pHeader, it->second);
        if (-1 == it->second.m_iFirstSerial)
        {
            it->second.m_iFirstSerial = pHeader->m_uSerial;
        }

        //数据接收序号出现环的情况
        if (it->second.m_iFirstSerial == 0xfffe)
        {
            it->second.m_iSerialGrow += 0xffff;
        }

        int iCurSerial = (pHeader->m_uSerial + it->second.m_iSerialGrow);
        if (iCurSerial == it->second.m_iFirstSerial)
        {
            it->second.m_iFirstSerial++;
            PushCompletePacket(it->second, strData);
            OnRecvComplete(it->second);
        }
        /**
        极偶然情况会走此处逻辑，比如首次接收的是第二个包
        比如前一个包超时清除，然后又收到
        */
        /**
        else if (iCurSerial < it->second.m_iFirstSerial)
        {
            it->second.m_iFirstSerial = iCurSerial;
            OnRecvComplete(it->second);
        }*/
        else if(iCurSerial > it->second.m_iFirstSerial)
        {
            PacketRecvDesc desc;
            desc.m_dwRecvTickCount = GetTickCount();
            PackageInterval interval;
            interval.m_iStartSerial = iCurSerial;
            interval.m_iPackageSize = pHeader->m_uSize;
            desc.m_interval = interval;
            desc.m_strContent = strData;
            //将封包区间插入区间集合，等待之前的封包接收
            InsertRecvInterval(desc, it->second.m_recvDescSet);
        }
    }
    return true;
}

bool Ups::OnRecvUpsAck(const string &strUnique, UpsHeader *pHeader)
{
    CScopedLocker lock(this);
    map<string, PackageSendCache>::iterator it = m_sendCache.find(strUnique);
    if (it == m_sendCache.end())
    {
        return false;
    }

    string strDbg = fmt("ack:%d\n", pHeader->m_uSerial);
    OutputDebugStringA(strDbg.c_str());

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

UpsHeader *Ups::EncodeHeader(UpsHeader *pHeader)
{
    pHeader->m_uMagic = htons(pHeader->m_uMagic);
    pHeader->m_uOpt = htons(pHeader->m_uOpt);
    pHeader->m_uSerial = htons(pHeader->m_uSerial);
    pHeader->m_uSize = htons(pHeader->m_uSize);
    return pHeader;
}

UpsHeader *Ups::DecodeHeader(UpsHeader *pHeader)
{
    pHeader->m_uMagic = ntohs(pHeader->m_uMagic);
    pHeader->m_uOpt = ntohs(pHeader->m_uOpt);
    pHeader->m_uSerial = ntohs(pHeader->m_uSerial);
    pHeader->m_uSize = ntohs(pHeader->m_uSize);
    return pHeader;
}

bool Ups::OnRecvPostData(const char *addr, unsigned short uPort, UpsHeader *pHeader, const string &strUnique, const string &strData)
{
    CScopedLocker lock(this);
    map<string, PackageRecvCache>::iterator it;
    if (m_recvCache.end() == (it = m_recvCache.find(strUnique)))
    {
        PackageRecvCache cache;
        cache.m_strIp = addr;
        cache.m_uPort = uPort;
        cache.m_iMagicNum = pHeader->m_uMagic;
        cache.m_strUnique = strUnique;
        m_recvCache[strUnique] = cache;
        it = m_recvCache.find(strUnique);
    }

    PushCompletePacket(it->second, strData);
    return true;
}

UpsHeader *Ups::PacketHeader(unsigned short uOpt, unsigned short uSerial, unsigned short uLength, UpsHeader *ptr)
{
    ptr->m_uMagic = m_uMagicNum;
    ptr->m_uOpt = uOpt;
    ptr->m_uSerial = uSerial;
    ptr->m_uSize = uLength;
    return EncodeHeader(ptr);
}

bool Ups::OnRecvUdpData(const char *addr, unsigned short uPort, const char *pData, int iLength)
{
    UpsHeader header;
    memcpy(&header, pData, sizeof(header));
    DecodeHeader(&header);

    if (IsValidMagic(header.m_uMagic))
    {
        return false;
    }

    string strUnique;
    int iDataLength = iLength - sizeof(UpsHeader);
    switch (header.m_uOpt) {
        case OPT_SEND_DATA:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_RECV);
                SendAck(addr, uPort, &header);
                OnRecvUpsData(addr, uPort, strUnique, &header, string(pData + sizeof(UpsHeader), iDataLength));
            }
            break;
        case OPT_POST_DATA:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_RECV);
                OnRecvPostData(addr, uPort, &header, strUnique, string(pData + sizeof(UpsHeader), iDataLength));
            }
            break;
        case OPT_REQUEST_ACK:
            {
                strUnique = GetConnectUnique(addr, uPort, MARK_SEND);
                OnRecvUpsAck(strUnique, &header);
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

        static int s_dbg = 0;
        string str = fmt("count2222:%d, size:%d, err:%d\n", s_dbg++, iRecvSize, WSAGetLastError());
        OutputDebugStringA(str.c_str());

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
    CScopedLocker lock(this);
    for (map<string, PackageSendCache>::iterator it = m_sendCache.begin() ; it != m_sendCache.end() ; it++)
    {
        for (vector<PacketSendDesc *>::iterator ij = it->second.m_sendSet.begin() ; ij != it->second.m_sendSet.end() ;)
        {
            PacketSendDesc *desc = *ij;
            if (dwCurCount - desc->m_stat.m_dwLastSendCount >= TIMESLICE_STAT)
            {
                desc->m_stat.m_dwLastSendCount = dwCurCount;
                SendToInternal(it->second.m_strRemoteIp, it->second.m_uReomtePort, desc->m_strContent);
                desc->m_stat.m_dwTestCount++;

                if (desc->m_stat.m_dwTestCount > MAX_TESTCOUNT)
                {
                    ij = it->second.m_sendSet.erase(ij);
                    continue;
                }
            }
            ij++;
        }
    }
    return true;
}

bool Ups::OnCheckPacketRecvStat()
{
    CScopedLocker lock(this);
    DWORD dwCurCount = GetTickCount();
    for (map<string, PackageRecvCache>::iterator it = m_recvCache.begin() ; it != m_recvCache.end() ; it++)
    {
        if (it->second.m_recvDescSet.size() > 0 && ((dwCurCount - it->second.m_recvDescSet.begin()->m_dwRecvTickCount) > MAX_RECVTIME))
        {
            it->second.m_iFirstSerial = it->second.m_recvDescSet.begin()->m_interval.m_iStartSerial;
            OnRecvComplete(it->second);
        }
    }
    return true;
}

unsigned short Ups::GetSendSerial()
{
    if (m_iSendSerial == 0xffff)
    {
        m_iSendSerial = 0;
        return m_iSendSerial;
    }
    return m_iSendSerial++;
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
        if (opt == OPT_SEND_DATA)
        {
            PacketHeader(opt, GetSendSerial(), iRealSize, &(ptr->m_header));
        }
        else
        {
            PacketHeader(opt, 0, iRealSize, &(ptr->m_header));
        }
        ptr->m_strContent.append((const char *)(&(ptr->m_header)), sizeof(UpsHeader));
        ptr->m_strContent += strData.substr(iPos, iRealSize);
        DecodeHeader(&(ptr->m_header));
        result.push_back(ptr);

        iPos += iRealSize;
        iFreeSize -= iRealSize;
    }

    if (iPos < (int)strData.size())
    {
        PacketSendDesc *ptr = new PacketSendDesc();
        unsigned short uSize = strData.size() - iPos;

        if (opt == OPT_SEND_DATA)
        {
            PacketHeader(opt, m_iSendSerial++, uSize, &(ptr->m_header));
        }
        else
        {
            PacketHeader(opt, 0, uSize, &(ptr->m_header));
        }
        ptr->m_strContent.append((const char *)(&(ptr->m_header)), sizeof(UpsHeader));
        ptr->m_strContent += strData.substr(iPos, strData.size() - iPos);
        DecodeHeader(&(ptr->m_header));
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

bool Ups::TestBindLocalPort(SOCKET sock, unsigned short uLocalPort)
{
    SOCKADDR_IN localAddr = {0};
    localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(uLocalPort);
    if (-1 == bind(m_udpSocket, (sockaddr *)&localAddr, sizeof(localAddr)))
    {
        return false;
    }
    return true;
}

bool Ups::UpsInit(unsigned short uLocalPort, bool bKeepAlive)
{
    if (m_bInit)
    {
        return false;
    }

    m_udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_uLocalPort = uLocalPort;

    if (uLocalPort > 0)
    {
        if (!TestBindLocalPort(m_udpSocket, uLocalPort))
        {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET;
            return false;
        }
    }
    else
    {
        /**
        udp特性决定随机端口也需要在接收数据前先绑定一个端口或则先发送一条数据
        否则在该socket上调用recvfrom会失败
        */
        int i = 0;
        for (i = 0 ; i < BIND_TEST_COUNT ; i++)
        {
            if (TestBindLocalPort(m_udpSocket, PORT_LOCAL_BASE + i))
            {
                m_uLocalPort = PORT_LOCAL_BASE + i;
                break;
            }
        }

        if (BIND_TEST_COUNT == i)
        {
            closesocket(m_udpSocket);
            m_udpSocket = INVALID_SOCKET;
            return false;
        }
    }

    m_bInit = true;
    //int nRecvBuf = 50 * 1024 * 1024;       //设置成5M
    //int res = setsockopt(m_udpSocket, SOL_SOCKET, SO_RCVBUF, (const char *)&nRecvBuf,sizeof(nRecvBuf));

    m_uMagicNum = GetMagicNumber();
    m_hStatEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hStopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    m_hRecvEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
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
    CloseHandle(m_hStopEvent);
    CloseHandle(m_hStatEvent);
    CloseHandle(m_hRecvEvent);
    m_hStopEvent = NULL;
    m_hStatEvent = NULL;
    m_hRecvEvent = NULL;

    CloseHandle(m_hStatThread);
    CloseHandle(m_hRecvThread);
    m_hStatThread = NULL;
    m_hRecvThread = NULL;
    m_iRecvPacketCount = 0;
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
        SendToInternal(addr, uPort, ptr->m_strContent);

        {
            CScopedLocker lock(this);
            PushCache(strUnique, addr, uPort, ptr);
            ptr->m_stat.m_dwLastSendCount = GetTickCount();
        }
    }
    return true;
}

int Ups::UpsRecv(string &strIp, USHORT &uPort, string &strData)
{
    WaitForSingleObject(m_hRecvEvent, INFINITE);
    CScopedLocker lock(this);
    for (map<string, PackageRecvCache>::iterator it = m_recvCache.begin() ; it != m_recvCache.end() ; it++)
    {
        if (!it->second.m_CompleteSet.empty())
        {
            strIp = it->second.m_strIp;
            uPort = it->second.m_uPort;
            strData = *(it->second.m_CompleteSet.begin());
            int iSize = strData.size();
            it->second.m_CompleteSet.erase(it->second.m_CompleteSet.begin());
            m_iRecvPacketCount--;

            string dbg = fmt("m_iRecvPacketCount sub:%d cache:%d\n", m_iRecvPacketCount, it->second.m_CompleteSet.size());
            OutputDebugStringA(dbg.c_str());

            if (m_iRecvPacketCount == 0)
            {
                ResetEvent(m_hRecvEvent);
            }
            return iSize;
        }
    }
    return 0;
}