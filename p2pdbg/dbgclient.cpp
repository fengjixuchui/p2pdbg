#include "dbgclient.h"

#pragma comment(lib, "Ws2_32.lib")

CDbgClient::CDbgClient()
{
    m_hWorkThread = NULL;
    m_bConnect = false;
}

CDbgClient::~CDbgClient()
{}

DWORD CDbgClient::RecvThread(LPVOID pParam)
{
    CDbgClient *ptr = (CDbgClient *)pParam;
    char buffer[4096] = {0};

    while (true)
    {
        int size = recv(ptr->m_clinetSock, buffer, 4096, 0);

        if (size <= 0)
        {
            break;
        }

        if (ptr->m_handler)
        {
            ptr->m_handler->onRecvData(ptr, string(buffer, size));
        }
    }

    ptr->Close();
    return 0;
}

bool CDbgClient::Connect(const string &strServIp, unsigned short uPort, int iTimeOut)
{
    unsigned long ul = 1;
    ioctlsocket(m_clinetSock, FIONBIO, (unsigned long*)&ul);
    SOCKADDR_IN servAddr ;
    servAddr.sin_family = AF_INET ;
    servAddr.sin_port = htons(uPort);
    servAddr.sin_addr.S_un.S_addr = inet_addr(strServIp.c_str());

    connect(m_clinetSock, (sockaddr *)&servAddr, sizeof(servAddr));

    struct timeval timeout;
    fd_set r;
    FD_ZERO(&r);
    FD_SET(m_clinetSock, &r);
    timeout.tv_sec = iTimeOut;
    timeout.tv_usec = 0;
    int ret = select(0, 0, &r, 0, &timeout);
    unsigned long ul1= 0 ;
    ioctlsocket(m_clinetSock, FIONBIO, (unsigned long*)&ul1);
    if (ret <= 0)
    {
        closesocket(m_clinetSock);
        m_clinetSock = INVALID_SOCKET;
        return false;
    }
    return true;
}

string CDbgClient::GetLocalIp()
{
    return m_strLocalIp;
}

bool CDbgClient::InitClient(const string &strServIp, unsigned short uPort, int iTimeOut, CDbgHandler *handler)
{
    bool bResult = false;
    m_clinetSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    do
    {
        struct sockaddr_in localAddr = {0};
        localAddr.sin_family = AF_INET;
        localAddr.sin_port = INADDR_ANY;
        localAddr.sin_addr.S_un.S_addr = INADDR_ANY;
        if (bind(m_clinetSock,(LPSOCKADDR)&localAddr, sizeof(localAddr)) == SOCKET_ERROR)
        {
            break;
        }

        if (!Connect(strServIp, uPort, iTimeOut))
        {
            break;
        }

        SOCKADDR_IN sockAddr = {0}; 
        int size = sizeof(sockAddr); 
        getsockname(m_clinetSock, (struct sockaddr *)&sockAddr, &size);
        m_strLocalIp = inet_ntoa(sockAddr.sin_addr);
        m_uLocalPort = ntohs(sockAddr.sin_port);

        m_hWorkThread = CreateThread(NULL, 0, RecvThread, this, 0, NULL);
        m_handler = handler; 
        bResult = true;
        m_bConnect = true;
    } while (false);

    if (!bResult && m_clinetSock != INVALID_SOCKET)
    {
        closesocket(m_clinetSock);
        m_clinetSock = INVALID_SOCKET;
    }
    return bResult;
}

bool CDbgClient::Close()
{
    m_bConnect = false;
    closesocket(m_clinetSock);
    m_clinetSock = INVALID_SOCKET;
    return true;
}

bool CDbgClient::SendData(const string &strData)
{
    if (m_clinetSock != INVALID_SOCKET)
    {
        send(m_clinetSock, strData.c_str(), strData.size(), 0);
        return true;
    }
    return false;
}