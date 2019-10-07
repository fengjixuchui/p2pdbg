#include "dbgserv.h"
#include <MSTcpIP.h>
#include "../ComLib/StrUtil.h"
#include "../ComLib/common.h"

CDbgServ::CDbgServ()
{
    m_bInit = false;
    m_servSock = INVALID_SOCKET;
    m_hWorkThread = NULL;
}

CDbgServ::~CDbgServ()
{}

DWORD CDbgServ::WorkThread(LPVOID pParam)
{
    CDbgServ *ptr = (CDbgServ *)pParam;

    FD_SET writeSet;
    FD_SET readSet;
    FD_SET errSet;
    vector<SOCKET>::const_iterator it;
    int iBufSize = (1024 * 1024 * 4);
    char *buffer = new char[iBufSize];
    DbgServHandler *handler = ptr->m_handler;

    while (true)
    {
        FD_ZERO(&readSet);
        FD_ZERO(&writeSet);
        FD_ZERO(&errSet);

        FD_SET(ptr->m_servSock, &readSet);
        FD_SET(ptr->m_servSock, &errSet);
        for (it = ptr->m_clientSet.begin() ; it != ptr->m_clientSet.end() ; it++)
        {
            FD_SET(*it, &readSet);
            FD_SET(*it, &writeSet);
            FD_SET(*it, &errSet);
        }

        int res = 0;
        if ((res = select(0, &readSet, NULL, &errSet, NULL)) != SOCKET_ERROR)
        {
            if (SOCKET_ERROR == res)
            {
                int err = WSAGetLastError();
                wstring wstrErr = FormatW(L"select err:%d", err);
                MessageBoxW(NULL, wstrErr.c_str(), L"socket err", 0);
                break;
            }

            if (FD_ISSET(ptr->m_servSock, &errSet))
            {
                int err = WSAGetLastError();
                wstring wstrErr = FormatW(L"m_servSock err:%d", err);
                MessageBoxW(NULL, wstrErr.c_str(), L"socket err", 0);
                break;
            }

            if (FD_ISSET(ptr->m_servSock, &readSet))
            {
                //新连接
                SOCKET client = accept(ptr->m_servSock, NULL, NULL);
                if (client != INVALID_SOCKET)
                {
                    ptr->m_clientSet.push_back(client);
                    handler->OnSocketAccept(client);
                }
            }

            for (it = ptr->m_clientSet.begin() ; it != ptr->m_clientSet.end() ;)
            {
                bool bDelete = false;
                SOCKET sock = *it;
                if (FD_ISSET(sock, &readSet))
                {
                    {
                        //接收数据
                        int iSize = recv(sock, buffer, iBufSize, 0);
                        if (iSize > 0)
                        {
                            //buffer[iSize] = 0x00; 
                            //dp(L"%hs", buffer);

                            string strResp;
                            handler->OnDataRecv(sock, string(buffer, iSize), strResp);
                            if (strResp.size() > 0)
                            {
                                send(sock, strResp.c_str(), strResp.size(), 0);
                            }
                        }
                        //接收出错
                        else
                        {
                            handler->OnSocketClose(sock);
                            closesocket(sock);
                            bDelete = true;
                        }
                    }
                }

                if (FD_ISSET(sock, &errSet))
                {
                    handler->OnSocketClose(sock);
                    closesocket(sock);
                    bDelete = true;
                }

                if (bDelete)
                {
                    it = ptr->m_clientSet.erase(it);
                    continue;
                }
                it++;
            }
        }
    }

    if (ptr->m_servSock != INVALID_SOCKET)
    {
        closesocket(ptr->m_servSock);
        ptr->m_servSock = INVALID_SOCKET;
    }
    delete []buffer;
    return 0;
}

bool CDbgServ::SetKeepAlive()
{
    tcp_keepalive live = {0};
    tcp_keepalive liveout = {0};
    live.keepaliveinterval = 500;
    live.keepalivetime = 3000;
    live.onoff = TRUE;
    int iKeepLive = 1;
    int iRet = setsockopt(m_servSock, SOL_SOCKET, SO_KEEPALIVE,(char *)&iKeepLive, sizeof(iKeepLive));
    if(iRet == 0){
        DWORD dw;
        if(WSAIoctl(m_servSock, SIO_KEEPALIVE_VALS, &live, sizeof(live), &liveout, sizeof(liveout), &dw, NULL, NULL)== SOCKET_ERROR)
        {
            return false;
        }
    }
    return true;
}

bool CDbgServ::InitDbgServ(unsigned short uLocalPort, DbgServHandler *handler)
{
    m_servSock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (INVALID_SOCKET == m_servSock)
    {
        return false;
    }

    bool bResult = false;
    do
    {
        struct sockaddr_in sin;
        sin.sin_family = AF_INET;
        sin.sin_port = htons(uLocalPort);
        sin.sin_addr.S_un.S_addr = INADDR_ANY;
        if(bind(m_servSock,(LPSOCKADDR)&sin,sizeof(sin)) == SOCKET_ERROR)
        {
            break;
        }

        //监听
        if(listen(m_servSock, 5) == SOCKET_ERROR)
        {
            break;
        }

        /**
        ULONG NonBlock = 1;
        if(ioctlsocket(m_servSock, FIONBIO, &NonBlock) == SOCKET_ERROR)
        {
            break;
        }
        */
        SetKeepAlive();
        m_handler = handler;
        m_hWorkThread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
        bResult = true;
        m_bInit = true;
    } while (false);

    if (!bResult && INVALID_SOCKET != m_servSock)
    {
        closesocket(m_servSock);
        m_servSock = INVALID_SOCKET;
    }
    return bResult;
}

bool CDbgServ::StopDbgServ() {
    if (!m_bInit)
    {
        return false;
    }

    if (INVALID_SOCKET != m_servSock)
    {
        closesocket(m_servSock);
        m_servSock = INVALID_SOCKET;
    }

    if (m_hWorkThread)
    {
        if (WAIT_TIMEOUT == WaitForSingleObject(m_hWorkThread, 3000))
        {
            TerminateThread(m_hWorkThread, 0);
        }
        CloseHandle(m_hWorkThread);
        m_hWorkThread = NULL;
    }
    m_bInit = false;
    return true;
}

bool CDbgServ::SendData(SOCKET sock, const string &strData)
{
    send(sock, strData.c_str(), strData.size(), 0);
    return true;
}