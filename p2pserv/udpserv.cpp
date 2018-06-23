#include "logic.h"

CUdpServ::CUdpServ() {
    m_servSocket = INVALID_SOCKET;
    m_bInit = false;
    m_uPort = 0;
    m_thread = NULL;
    m_pfn = NULL;
}

DWORD CUdpServ::WorkThread(LPVOID pParam) {
    CUdpServ *ptr = (CUdpServ *)pParam;
    char buffer[4096];
    SOCKADDR_IN clientAddr = {0};
    int length = sizeof(buffer);
    int addrLength = 0;

    while (true) {
        memset(&clientAddr, 0, sizeof(SOCKADDR_IN));
        addrLength = sizeof(clientAddr);
        length = recvfrom(ptr->m_servSocket, buffer, sizeof(buffer), 0, (sockaddr *)&clientAddr, &addrLength);

        if (length <= 0) {
            break;
        }

        if (ptr->m_pfn)
        {
            string strAddr = inet_ntoa(clientAddr.sin_addr);
            USHORT uPort = ntohs(clientAddr.sin_port);
            ptr->m_pfn(string(buffer, length), strAddr, uPort);
        }
    }
    return 0;
}

string CUdpServ::GetLocalIp()
{
    return m_strLocalIp;
}

string CUdpServ::GetLocalIpInternal()
{
    SOCKET tmp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    struct sockaddr_in remoteAddr;
    struct sockaddr_in localAddr;

    memset (&remoteAddr, 0, sizeof(struct sockaddr));  
    remoteAddr.sin_family = AF_INET;  
    remoteAddr.sin_port = htons (1234);
    remoteAddr.sin_addr.S_un.S_addr = inet_addr("1.2.3.4");

    int len = sizeof(sockaddr_in);
    connect(tmp, (struct sockaddr *)&remoteAddr, len);
    getsockname(tmp, (struct sockaddr *)&localAddr, &len);
    closesocket(tmp);
    return inet_ntoa(localAddr.sin_addr);
}

bool CUdpServ::StartServ(USHORT uPort, pfnOnRecv pfn) {
    if (m_bInit){
        return true;
    }

    m_servSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_uPort = uPort;
    m_pfn = pfn;

    SOCKADDR_IN addrServ = {0};
    addrServ.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    addrServ.sin_family = AF_INET;
    addrServ.sin_port = htons(uPort);

    int e = bind(m_servSocket, (sockaddr *)&addrServ, sizeof(addrServ));
    m_bInit = true;
    m_strLocalIp = GetLocalIpInternal();
    m_thread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
    return true;
}

bool CUdpServ::SendTo(const string &strAddr, USHORT uPort, const string &strData)
{
    SOCKADDR_IN addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = inet_addr(strAddr.c_str());
    addr.sin_port = htons(uPort);
    sendto(m_servSocket, strData.c_str(), strData.length(), 0, (const sockaddr *)&addr, sizeof(addr));
    return true;
}