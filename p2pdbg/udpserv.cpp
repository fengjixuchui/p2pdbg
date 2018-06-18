#include <WinSock2.h>
#include "udpserv.h"
#include "logic.h"

#pragma comment(lib, "Ws2_32.lib")

CUdpServ::CUdpServ() {
    m_clientSock = INVALID_SOCKET;
    m_bInit = false;
    m_uLocalPort = 0;
    m_uServPort = 0;
    m_hWorkThread = NULL;
    m_pfn = NULL;
    m_strServIp = IP_SERV;
    m_uServPort = PORT_SERV;
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
        length = recvfrom(ptr->m_clientSock, buffer, sizeof(buffer), 0, (sockaddr *)&clientAddr, &addrLength);

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
    struct sockaddr_in remoteAddr;
    struct sockaddr_in localAddr;

    memset (&remoteAddr, 0, sizeof(struct sockaddr));  
    remoteAddr.sin_family = AF_INET;  
    remoteAddr.sin_port = htons (PORT_SERV);
    remoteAddr.sin_addr.S_un.S_addr = inet_addr(IP_SERV);

    int len = sizeof(sockaddr_in);
    connect (m_clientSock, (struct sockaddr *)&remoteAddr, len);
    getsockname(m_clientSock, (struct sockaddr *)&localAddr, &len);
    return inet_ntoa(localAddr.sin_addr);
}

bool CUdpServ::StartServ(USHORT uLocalPort, pfnOnRecv pfn) {
    if (m_bInit){
        return true;
    }

    m_clientSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    m_uLocalPort = uLocalPort;
    m_pfn = pfn;

    SOCKADDR_IN localAddr = {0};
    localAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(PORT_LOCAL);
    bind(m_clientSock, (sockaddr *)&localAddr, sizeof(localAddr));

    m_strLocalIp = GetLocalIp();
    m_bInit = true;
    m_hWorkThread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
    return true;
}
bool CUdpServ::SendTo(const string &strIp, USHORT uPort, const string &strData)
{
    SOCKADDR_IN addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.S_un.S_addr = inet_addr(strIp.c_str());
    addr.sin_port = htons(uPort);
    sendto(m_clientSock, strData.c_str(), strData.length(), 0, (const sockaddr *)&addr, sizeof(addr));
    return true;
}