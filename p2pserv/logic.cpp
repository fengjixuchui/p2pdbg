#include <WinSock2.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <iphlpapi.h>
#include "../ComLib/json/json.h"
#include "../ComLib/StrUtil.h"
#include "logic.h"
#include "cmddef.h"

using namespace Json;
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "shlwapi.lib")

CWorkLogic *CWorkLogic::ms_inst = NULL;
#define DOMAIN_SERV         "p2pdbg.picp.io"
#define PORT_MSG_SERV       9971
#define PORT_FTP_SERV       9981

bool DbgServMsgHandler::OnSocketAccept(SOCKET sock)
{
    return CWorkLogic::GetInstance()->OnMsgSocketAccept(sock);
}

bool DbgServMsgHandler::OnDataRecv(SOCKET sock, const string &strRecv, string &strResp)
{
    return CWorkLogic::GetInstance()->OnMsgDataRecv(sock, strRecv, strResp);
}

bool DbgServMsgHandler::OnSocketClose(SOCKET sock)
{
    return CWorkLogic::GetInstance()->OnMsgSocketClose(sock);
}

bool DbgServFtpHandler::OnSocketAccept(SOCKET sock)
{
    return CWorkLogic::GetInstance()->OnFtpSocketAccept(sock);
}

bool DbgServFtpHandler::OnDataRecv(SOCKET sock, const string &strRecv, string &strResp)
{
    return CWorkLogic::GetInstance()->OnFtpDataRecv(sock, strRecv, strResp);
}

bool DbgServFtpHandler::OnSocketClose(SOCKET sock)
{
    return CWorkLogic::GetInstance()->OnFtpSocketClose(sock);
}

CWorkLogic::CWorkLogic()
{
    m_bInit = false;
    m_hWorkThread = NULL;
    m_hKeepAliveThread = NULL;
}

bool CWorkLogic::StartWork(USHORT uMsgPort, USHORT uFtpPort)
{
    if (m_bInit)
    {
        return true;
    }

    m_bInit = true;
    m_dbgServ.InitDbgServ(uMsgPort, &m_MsgHandler);
    m_ftpServ.InitDbgServ(uFtpPort, &m_FtpHandler);
    m_hFtpNotify = CreateEventA(NULL, FALSE, FALSE, NULL);
    m_hWorkThread = CreateThread(NULL, 0, ServWorkThread, this, 0, NULL);
    m_hKeepAliveThread = CreateThread(NULL, 0, KeepAliveThread, this, 0, NULL);
    m_hFtpPushThread = CreateThread(NULL, 0, FtpPushThread, this, 0, NULL);
    return true;
}

bool CWorkLogic::OnMsgDataRecv(SOCKET sock, const string &strData, string &strResp)
{
    CScopedLocker lock(&m_DataLock);
    ClientInfo *pClientInfo = GetClientFromMsgSock(sock);
    if (pClientInfo == NULL)
    {
        return false;
    }
    pClientInfo->m_strMsgCache.append(strData);

    size_t lastPos = 0;
    size_t pos = pClientInfo->m_strMsgCache.find(CMD_START_MARK);
    if (pos != 0)
    {
        pClientInfo->m_strMsgCache.clear();
        return false;
    }

    while (true)
    {
        size_t iStart = lstrlenA(CMD_START_MARK);
        pos = pClientInfo->m_strMsgCache.find(CMD_FINISH_MARK, iStart);
        if (pos == string::npos)
        {
            break;
        }

        size_t iEnd = pos;
        string strSub = pClientInfo->m_strMsgCache.substr(iStart, iEnd - iStart);
        pClientInfo->m_strMsgCache.erase(0, iEnd + lstrlenA(CMD_FINISH_MARK));
        GetInstance()->OnMsgSingleData(sock, strSub);
    }
    return true;
}

bool CWorkLogic::OnMsgSocketClose(SOCKET sock)
{
    CScopedLocker lock(&m_DataLock);
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ; it++)
    {
        ClientInfo *ptr = *it;
        if (ptr->m_MsgSock == sock)
        {
            closesocket(ptr->m_MsgSock);
            closesocket(ptr->m_FtpSock);
            m_clientSet.erase(it);
            delete ptr;
            return true;
        }
    }
    return true;
}

CWorkLogic *CWorkLogic::GetInstance(){
    if (ms_inst == NULL)
    {
        ms_inst = new CWorkLogic();
    }
    return ms_inst;
}

void CWorkLogic::OnGetClient(SOCKET sock, const Value &vJson)
{
    string strDataType = vJson.get("dataType", "").asString();
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asUInt();

    Value vClient;
    Value vArry(arrayValue);
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ; it++)
    {
        ClientInfo *ptr = *it;
        //注册过的客户端
        if (ptr->m_strUnique.empty())
        {
            continue;
        }

        vClient["unique"] = ptr->m_strUnique;
        vClient["clientDesc"] = ptr->m_strClientDesc;
        vClient["ipInternal"] = ptr->m_strIpInternal;
        vClient["portInternal"] = ptr->m_uPortInternal;
        vClient["ipExternal"] = ptr->m_strIpExternal;
        vClient["portExternal"] = ptr->m_uPortExternal;
        vClient["startTime"] = ptr->m_strStartTime;
        vArry.append(vClient);
    }
    Value vReply = makeReply(id, 0, vArry);
    SendData(sock, vReply);
}

/**
终端到服务端的心跳
{
    "dataType":"heartbeat_c2s",
    "unique":"终端标识",
    "time":"发送时间"

    "clientDesc":"设备描述",
    "ipInternal":"内部ip",
    "portInternal":"内部的端口"
}
*/
void CWorkLogic::OnClientHeartbeat(SOCKET sock, const Value &vJson)
{
    string strUnique = vJson.get("unique", "").asString();
    string strInternalIp = vJson.get("ipInternal", "").asString();
    int uInternalPort = vJson.get("portInternal", 0).asUInt();
    string strClientDesc = vJson.get("clientDesc", "").asString();
    int id = vJson.get("id", 0).asUInt();

    ClientInfo *pClientInfo = GetClientFromMsgSock(sock);
    {
        SOCKADDR_IN sockAddr = {0}; 
        int size = sizeof(sockAddr); 
        getpeername(sock, (struct sockaddr *)&sockAddr, &size);

        pClientInfo->m_strUnique = strUnique;
        pClientInfo->m_MsgSock = sock;
        pClientInfo->m_strClientDesc = strClientDesc;
        pClientInfo->m_strIpInternal = strInternalIp;
        pClientInfo->m_uPortInternal = uInternalPort;
        pClientInfo->m_strIpExternal = inet_ntoa(sockAddr.sin_addr);
        pClientInfo->m_uPortExternal = ntohs(sockAddr.sin_port);
        pClientInfo->m_uLastHeartbeat = GetTickCount();
    }
    Value vReply = makeReply(id, 0, Value(objectValue));
    SendData(sock, vReply);
}

Value CWorkLogic::makeReply(int id, int iStat, const Value &vContent)
{
    Value reply;
    reply["dataType"] = CMD_REPLY;
    reply["id"] = id;
    reply["stat"] = iStat;
    reply["content"] = vContent;
    return reply;
}

bool CWorkLogic::SendData(SOCKET sock, const Value &vData)
{
    string str = CMD_START_MARK;
    str += FastWriter().write(vData);
    str += CMD_FINISH_MARK;
    m_dbgServ.SendData(sock, str);
    return true;
}

void CWorkLogic::OnTransData(SOCKET sock, const Value &vJson)
{
    string strDst = vJson["dst"].asString();

    ClientInfo *pClinet = GetClientFromUnique(strDst);
    if (NULL == pClinet)
    {
        return;
    }
    SendData(pClinet->m_MsgSock, vJson);
}

void CWorkLogic::OnConnentClient(SOCKET sock, const Value &vJson)
{
    string strDst = vJson["dst"].asString();
    int id = vJson["id"].asInt();

    ClientInfo *pClient = GetClientFromUnique(strDst);
    Value result;
    Value content(objectValue);
    if (NULL == pClient)
    {
        content["desc"] = "client not find";
        result = makeReply(id, 1, content);
    }
    else
    {
        content["clientDesc"] = pClient->m_strClientDesc;
        result = makeReply(id, 0, content);
    }
    SendData(sock, result);
}

FtpInfo *CWorkLogic::GetFtpInfoBySocket(SOCKET sock)
{
    for (list<FtpInfo *>::const_iterator it = m_ftpSet.begin() ; it != m_ftpSet.end() ; it++)
    {
        FtpInfo *ptr = *it;
        if (ptr->m_ftpSock == sock)
        {
            return ptr;
        }
    }

    FtpInfo *pNewFtpInfo = new FtpInfo();
    pNewFtpInfo->m_ftpSock = sock;
    m_ftpSet.push_back(pNewFtpInfo);
    return pNewFtpInfo;
}

void CWorkLogic::OnFtpTransferStat(FtpInfo *pFtpInfo, string &strData)
{
    pFtpInfo->m_dwLastCount = GetTickCount();
    ULONGLONG uNeedSize = pFtpInfo->m_iFileSize - pFtpInfo->m_iRecvSize;
    DWORD dwWrite = 0;
    if (uNeedSize >= strData.size())
    {
        WriteFile(pFtpInfo->m_hFileHandle, strData.c_str(), strData.size(), &dwWrite, NULL);
        pFtpInfo->m_iRecvSize += strData.size();
        strData.clear();
    }
    else
    {
        //粘包情况
        WriteFile(pFtpInfo->m_hFileHandle, strData.c_str(), (DWORD)uNeedSize, &dwWrite, NULL);
        pFtpInfo->m_iRecvSize += (int)uNeedSize;
        strData.erase(0, (unsigned int)uNeedSize);
    }

    //文件传输结束
    if (pFtpInfo->m_iFileSize == pFtpInfo->m_iRecvSize)
    {
        pFtpInfo->m_eStat = em_ftp_succ;
        CloseHandle(pFtpInfo->m_hFileHandle);
        pFtpInfo->m_hFileHandle = INVALID_HANDLE_VALUE;

        FtpFile *ftpFile = new FtpFile();
        ftpFile->m_bCompress = pFtpInfo->m_bCompress;
        ftpFile->m_iFileSize = pFtpInfo->m_iFileSize;
        ftpFile->m_strDesc = pFtpInfo->m_strDesc;
        ftpFile->m_strDst = pFtpInfo->m_strDst;
        ftpFile->m_strFileName = pFtpInfo->m_strFileName;
        ftpFile->m_strLocalPath = pFtpInfo->m_strLocalPath;
        ftpFile->m_strSrc = pFtpInfo->m_strSrc;

        srand(GetTickCount());
        ftpFile->m_strFtpUnique = FormatA(
            "%04x%04x%04x%04x",
            rand() % 0xffff,
            rand() % 0xffff,
            rand() % 0xffff,
            rand() % 0xffff
            );
        m_fileSet.push_back(ftpFile);
        SetEvent(m_hFtpNotify);
        pFtpInfo->reset();
    }
}

ClientInfo *CWorkLogic::GetClientFromMsgSock(SOCKET sock)
{
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ; it++)
    {
        ClientInfo *ptr = *it;
        if (ptr->m_MsgSock == sock)
        {
            return ptr;
        }
    }

    //没有自动进行创建
    ClientInfo *client = new ClientInfo;

    SYSTEMTIME time = {0};
    GetLocalTime(&time);
    client->m_strStartTime = FormatA(
        "%04d-%02d-%02d %02d:%02d:%02d",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond
        );
    client->m_MsgSock = sock;
    m_clientSet.push_back(client);
    return client;
}

ClientInfo *CWorkLogic::GetClientFromUnique(const string &strUnique)
{
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ; it++)
    {
        ClientInfo *ptr = *it;
        if (ptr->m_strUnique == strUnique)
        {
            return ptr;
        }
    }
    return NULL;
}

void CWorkLogic::OnMsgSingleData(SOCKET sock, const string &strData)
{
    Reader reader;
    Value vJson;

    try {
        reader.parse(strData, vJson);
        if (vJson.type() != objectValue)
        {
            return;
        }

        string strDataType = vJson.get("dataType", "").asString();
        int id = vJson.get("id", 0).asUInt();
        //获取用户列表
        if (strDataType == CMD_C2S_GETCLIENTS)
        {
            OnGetClient(sock, vJson);
        }
        //终端到服务端心跳
        else if (strDataType == CMD_C2S_HEARTBEAT)
        {
            OnClientHeartbeat(sock, vJson);
        }
        //服务端转发数据
        else if (strDataType == CMD_C2S_TRANSDATA)
        {
            OnTransData(sock, vJson);
        }
        //连接调试终端
        else if (strDataType == CMD_C2S_CONNECT)
        {
            OnConnentClient(sock, vJson);
        }
        //未定义的数据
        else
        {
            Value reply = makeReply(id, 1, Value());
            SendData(sock, FastWriter().write(reply));
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

void CWorkLogic::OnCheckClientStat() {
    CScopedLocker lock(&m_DataLock);
    DWORD dwCurCount = GetTickCount();
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ;)
    {
        ClientInfo *pClinet = *it;
        if (pClinet->m_strUnique.empty())
        {
            it++;
            continue;
        }

        if ((dwCurCount - pClinet->m_uLastHeartbeat) >= TIMECOUNT_MAX)
        {
            closesocket(pClinet->m_MsgSock);
            closesocket(pClinet->m_FtpSock);
            it = m_clientSet.erase(it);
            delete pClinet;
            continue;
        }

        //ftp传送状态清理
        if (pClinet->m_iTransMode == 1 && (dwCurCount - pClinet->m_fileInfo.m_dwLastRecvCount) >= (1000 * 10))
        {
            pClinet->m_iTransMode = 0;
            pClinet->m_fileInfo.m_uFileSize = 0;
            pClinet->m_fileInfo.m_uRecvSize = 0;
        }
        it++;
    }
}

DWORD CWorkLogic::ServWorkThread(LPVOID pParam){
    CWorkLogic *ptr = (CWorkLogic *)pParam;

    while (true)
    {
        Sleep(5 * 1000);
        ptr->OnCheckClientStat();
    }
    return 0;
}

DWORD CWorkLogic::KeepAliveThread(LPVOID pParam) {
    CDbgServ *pMsgServ = &(CWorkLogic::GetInstance()->m_dbgServ);
    CDbgServ *pFtpServ = &(CWorkLogic::GetInstance()->m_ftpServ);

    while (true) {
        Sleep(3 * 1000);

        if (!CWorkLogic::GetInstance()->IsTcpPortAlive(PORT_MSG_SERV))
        {
            pMsgServ->StopDbgServ();
            pMsgServ->InitDbgServ(PORT_MSG_SERV, &(CWorkLogic::GetInstance()->m_MsgHandler));
        }

        if (!CWorkLogic::GetInstance()->IsTcpPortAlive(PORT_FTP_SERV))
        {
            pFtpServ->StopDbgServ();
            pFtpServ->InitDbgServ(PORT_FTP_SERV, &(CWorkLogic::GetInstance()->m_FtpHandler));
        }
    }
    return 0;
}

void CWorkLogic::OnPushFtpFile() {
    for (list<FtpFile *>::const_iterator it = m_fileSet.begin() ; it != m_fileSet.end() ;)
    {
        FtpFile *pFile = *it;
        bool bDelete = false;
        for (list<ClientInfo *>::const_iterator ij = m_clientSet.begin() ; ij != m_clientSet.end() ; ij++)
        {
            ClientInfo *pClient = *ij;
            /**
            推送ftp文件
            */
            if (pClient->m_strUnique == pFile->m_strDst)
            {
                Value vNotify;
                vNotify["dataType"] = CMD_C2S_FTPFILE_NOTIFY;
                vNotify["ftpUnique"] = pFile->m_strFtpUnique;
                SendData(pClient->m_MsgSock, vNotify);
                pFile->m_iNotifyCount++;
                break;
            }
        }

        if (pFile->m_iNotifyCount >= 5)
        {
            it = m_fileSet.erase(it);
        } else {
            it++;
        }
    }
}

DWORD CWorkLogic::FtpPushThread(LPVOID pParam) {
    HANDLE hNotify = CWorkLogic::GetInstance()->m_hFtpNotify;
    while (true) {
        WaitForSingleObject(hNotify, 5000);

        CScopedLocker lock(&CWorkLogic::GetInstance()->m_DataLock);
        CWorkLogic::GetInstance()->OnPushFtpFile();
    }
    return 0;
}

string CWorkLogic::GetIpFromDomain(const string &strDomain)
{
    HOSTENT *host_entry = gethostbyname(strDomain.c_str());
    if(host_entry != NULL)
    {
        return FormatA(
            "%d.%d.%d.%d",
            (host_entry->h_addr_list[0][0]&0x00ff),
            (host_entry->h_addr_list[0][1]&0x00ff),
            (host_entry->h_addr_list[0][2]&0x00ff),
            (host_entry->h_addr_list[0][3]&0x00ff)
            );
    }
    return "";
}

bool CWorkLogic::Connect(SOCKET sock, const string &strServIp, unsigned short uPort, int iTimeOut)
{
    unsigned long ul = 1;
    ioctlsocket(sock, FIONBIO, (unsigned long*)&ul);
    SOCKADDR_IN servAddr ;
    servAddr.sin_family = AF_INET ;
    servAddr.sin_port = htons(uPort);
    servAddr.sin_addr.S_un.S_addr = inet_addr(strServIp.c_str());

    connect(sock, (sockaddr *)&servAddr, sizeof(servAddr));

    struct timeval timeout;
    fd_set r;
    FD_ZERO(&r);
    FD_SET(sock, &r);
    timeout.tv_sec = iTimeOut / 1000;
    timeout.tv_usec = 0;
    int ret = select(0, 0, &r, 0, &timeout);
    unsigned long ul1= 0 ;
    ioctlsocket(sock, FIONBIO, (unsigned long*)&ul1);
    if (ret <= 0)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool CWorkLogic::IsTcpPortAlive(USHORT uPort) {
    bool bResult = false;
    DWORD dwSize = 0;
    GetTcpTable(NULL, &dwSize, TRUE);
    dwSize += 1024;

    char *buffer = new char[dwSize];
    GetTcpTable((PMIB_TCPTABLE)buffer, &dwSize, TRUE);

    if (dwSize > 0)
    {
        PMIB_TCPTABLE pTable = (PMIB_TCPTABLE)buffer;
        for (int i = 0 ; i < (int)pTable->dwNumEntries ; i++)
        {
            MIB_TCPROW *node = pTable->table + i;
            if (node->dwLocalPort == htons(uPort) && node->dwState == MIB_TCP_STATE_LISTEN)
            {
                bResult = true;
                break;
            }
        }
    }

    if (buffer)
    {
        delete []buffer;
    }
    return bResult;
}

bool CWorkLogic::OnMsgSocketAccept(SOCKET sock)
{
    CScopedLocker lock(&m_DataLock);
    GetClientFromMsgSock(sock);
    return true;
}

//文件回调接口
bool CWorkLogic::OnFtpSocketAccept(SOCKET sock)
{
    CScopedLocker lock(&m_DataLock);
    return true;
}

ClientInfo *CWorkLogic::GetClientFromFtpSock(SOCKET sock)
{
    for (list<ClientInfo *>::const_iterator it = m_clientSet.begin() ; it != m_clientSet.end() ; it++)
    {
        ClientInfo *pClient = *it;
        if (pClient->m_FtpSock == sock)
        {
            return pClient;
        }
    }
    return NULL;
}

bool CWorkLogic::OnFtpDataRecv(SOCKET sock, const string &strData, string &strResp)
{
    CScopedLocker lock(&m_DataLock);
    string strRecv = strData;
    string strSrcClient;
    FtpInfo *pFtpInfo = NULL;
    pFtpInfo = GetFtpInfoBySocket(sock);
    pFtpInfo->m_strRecvCache.append(strData);
    while (true)
    {
        if (pFtpInfo->m_eStat == em_ftp_start || pFtpInfo->m_eStat == em_ftp_transing)
        {
            OnFtpTransferStat(pFtpInfo, pFtpInfo->m_strRecvCache);
        }

        if (pFtpInfo->m_strRecvCache.empty())
        {
            break;
        }

        /**
        能到这说明没有处于文件接收状态
        */
        size_t iStart = lstrlenA(CMD_START_MARK);
        if (0 != pFtpInfo->m_strRecvCache.find(CMD_START_MARK))
        {
            pFtpInfo->m_strRecvCache.clear();
            break;
        }

        size_t pos = pFtpInfo->m_strRecvCache.find(CMD_FINISH_MARK, iStart);
        if (pos == string::npos)
        {
            break;
        }

        size_t iEnd = pos;
        string strSub = pFtpInfo->m_strRecvCache.substr(iStart, iEnd - iStart);
        pFtpInfo->m_strRecvCache.erase(0, iEnd + lstrlenA(CMD_FINISH_MARK));
        GetInstance()->OnFtpSingleData(sock, strSub);
    }
    return true;
}

void CWorkLogic::OnFtpRegister(SOCKET sock, const Value &vJson)
{
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asInt();

    ClientInfo *pClinet = GetClientFromUnique(strUnique);
    if (pClinet == NULL)
    {
        return;
    }
    pClinet->m_strUnique = strUnique;
    pClinet->m_FtpSock = sock;
    Value vReply = makeReply(id, 0, Value(objectValue));
    SendData(sock, vReply);
}

string CWorkLogic::GetLoaclFtpPath(string strDstUnique, bool bCompress) {
    char szPath[256] = {0};
    GetModuleFileNameA(NULL, szPath, 256);
    PathAppendA(szPath, "..\\dbgServCache");
    PathAppendA(szPath, strDstUnique.c_str());

    SHCreateDirectoryExA(NULL, szPath, NULL);

    SYSTEMTIME time = {0};
    GetLocalTime(&time);
    string strFileName = FormatA(
        "%04d%02d%02d_%02d%02d%02d_%03d",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        time.wMilliseconds
        );

    if (bCompress)
    {
        strFileName += ".zip";
    }
    PathAppendA(szPath, strFileName.c_str());
    return szPath;
}

/**
<Protocol Start>
{
    "dataType":"ftpTransfer",
    "id":112233,
    "fileUnique":"文件标识",
    "src":"文件发送方",
    "dst":"文件接收方",
    "desc":"文件描述",
    "fileSize":112233,
    "fileName":"文件名"
}
<Protocol Finish>
*/
void CWorkLogic::OnFtpTransferBegin(SOCKET sock, const Value &vJson)
{
    int id = vJson.get("id", 0).asInt();
    string strSrc = vJson.get("src", "").asString();
    string strDst = vJson.get("dst", "").asString();
    string strDesc = vJson.get("desc", "").asString();
    int iFileSize = vJson.get("fileSize", 0).asUInt();
    string strFileName = vJson.get("fileName", "").asString();

    /**
    落盘并缓存ftp数据，接收完成后推送给目标调试器
    */
    FtpInfo *pFtpInfo = GetFtpInfoBySocket(sock);
    pFtpInfo->m_ftpSock = sock;
    pFtpInfo->m_strSrc = UtoA(strSrc);
    pFtpInfo->m_strDst = UtoA(strDst);
    pFtpInfo->m_strDesc = UtoA(strDesc);
    pFtpInfo->m_strFileName = UtoA(strFileName);
    pFtpInfo->m_iFileSize = iFileSize;
    pFtpInfo->m_bCompress = vJson.get("compress", true).asBool();
    pFtpInfo->m_strLocalPath = GetLoaclFtpPath(pFtpInfo->m_strSrc, pFtpInfo->m_bCompress);
    pFtpInfo->m_hFileHandle = CreateFileA(
        pFtpInfo->m_strLocalPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_NEW,
        0,
        NULL
        );
    pFtpInfo->m_eStat = em_ftp_start;
    m_ftpSet.push_back(pFtpInfo);
}

void CWorkLogic::SendFile(SOCKET sock, const string &strFilePath) {
    HANDLE hFile = CreateFileA(
        strFilePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        0,
        NULL
        );

    if (hFile == INVALID_HANDLE_VALUE || NULL == hFile)
    {
        return;
    }

    char buffer[1024];
    DWORD dwReadSize = 0;
    while (true) {
        dwReadSize = sizeof(buffer);
        ReadFile(hFile, buffer, dwReadSize, &dwReadSize, NULL);

        if (dwReadSize <= 0)
        {
            break;
        }
        m_ftpServ.SendData(sock, string(buffer, dwReadSize));
    }
    CloseHandle(hFile);
}

void CWorkLogic::OnFtpGetFile(SOCKET sock, const Value &vJson) {
    CScopedLocker lock(&m_DataLock);
    int id = vJson.get("id", 0).asInt();
    string strFtpUnique = vJson.get("ftpUnique", "").asString();

    for (list<FtpFile *>::const_iterator it = m_fileSet.begin() ; it != m_fileSet.end() ; it++)
    {
        FtpFile *ptr = *it;
        if (ptr->m_strFtpUnique == strFtpUnique)
        {
            /**
            <Protocol Start>
            {
                "dataType":"ftpTransfer",
                "id":112233,
                "fileUnique":"文件标识",
                "src":"文件发送方",
                "dst":"文件接收方",
                "desc":"文件描述",
                "fileSize":112233,
                "fileName":"文件名"
            }
            <Protocol Finish>
            */
            Value vContent;
            vContent["dataType"] = CMD_FTP_TRANSFER;
            vContent["id"] = id;
            vContent["ftpUnique"] = strFtpUnique;
            vContent["src"] = ptr->m_strSrc;
            vContent["dst"] = ptr->m_strDst;
            vContent["desc"] = ptr->m_strDesc;
            vContent["fileSize"] = ptr->m_iFileSize;
            vContent["fileName"] = ptr->m_strFileName;
            vContent["compress"] = (int)ptr->m_bCompress;
            SendData(sock, vContent);
            SendFile(sock, ptr->m_strLocalPath);
            DeleteFileA(ptr->m_strLocalPath.c_str());
            delete ptr;
            m_fileSet.erase(it);
            return;
        }
    }
}

void CWorkLogic::OnFtpSingleData(SOCKET sock, const string &strData)
{
    Reader reader;
    Value vJson;

    try {
        reader.parse(strData, vJson);
        if (vJson.type() != objectValue)
        {
            return;
        }

        string strDataType = vJson.get("dataType", "").asString();
        int id = vJson.get("id", 0).asUInt();
        //注册ftp客户端 优化后不再需要注册，但是为了向前兼容暂时保留
        if (strDataType == CMD_FTP_REGISTER)
        {
            OnFtpRegister(sock, vJson);
        } else if (strDataType == CMD_FTP_TRANSFER)
        {
            OnFtpTransferBegin(sock, vJson);
        } else if (strDataType == CMD_FTP_GETFILE)
        {
            OnFtpGetFile(sock, vJson);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

bool CWorkLogic::OnFtpSocketClose(SOCKET sock)
{
    return true;
}