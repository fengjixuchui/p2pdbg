#include <WinSock2.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include "../ComLib/json/json.h"
#include "../ComLib/StrUtil.h"
#include "logic.h"
#include "cmddef.h"
#include "dbgview.h"

#pragma comment(lib, "shell32.lib")

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = NULL;

void CDbgMsgHanler::onRecvData(const string &strData)
{
    CWorkLogic::GetInstance()->OnRecvMsgData(strData);
}

void CDbgFtpHandler::onRecvData(const string &strData)
{
    CWorkLogic::GetInstance()->OnRecvFtpData(strData);
}


CWorkLogic::CWorkLogic()
{
    m_hCompleteNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_bInit = false;
    m_bConnectSucc = false;
}

bool CWorkLogic::RegisterMsgServSyn()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_HEARTBEAT);
    vJson["clientDesc"] = GetDevDesc();
    vJson["ipInternal"] = m_strLocalIp;
    vJson["portInternal"] = m_uLocalPort;
    int id = vJson["id"].asUInt();
    string strResult;
    return SendMsgForResult(id, vJson, strResult, 5000);
}

mstring CWorkLogic::GetDevUnique(){
    char szBuffer[MAX_PATH] = {0};
    DWORD dwSize = sizeof(szBuffer);
    SHGetValueA(HKEY_LOCAL_MACHINE, "Software\\p2pdbg", "devUnique", NULL, szBuffer, &dwSize);

    if (szBuffer[0])
    {
        return szBuffer;
    }
    srand(GetTickCount());
    mstring strDevUnique = FormatW(
        L"%x%x%x%x%x%x%x%x-%x%x%x%x-%x%x%x%x-%x%x%x%x%x%x%x%x",
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf, 
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf
        );
    SHSetValueA(HKEY_LOCAL_MACHINE, "Software\\p2pdbg", "devUnique", REG_SZ, strDevUnique.c_str(), strDevUnique.size());
    return strDevUnique;
}

bool CWorkLogic::StartWork()
{
    if (m_bInit)
    {
        return true;
    }

    m_bInit = true;
    m_bFtpTransfer = false;;
    m_strDevUnique = GetDevUnique();
    m_uLocalPort = PORT_LOCAL;
    //m_strServIp = GetIpFromDomain(DOMAIN_SERV);
    m_strServIp = "192.168.0.103";
    m_MsgServ.InitClient(m_strServIp, PORT_MSG_SERV, 1, &m_MsgHandler);
    m_FtpServ.InitClient(m_strServIp, PORT_FTP_SERV, 1, &m_FtpHandler);
    m_strLocalIp = m_MsgServ.GetLocalIp();

    if (!RegisterMsgServSyn())
    {
        return false;
    }
    m_hWorkThread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
    return true;
}

string CWorkLogic::GetDevDesc()
{
    string strDesc = DBGDESC;
    strDesc += "-";
    strDesc += m_strLocalIp;
    return strDesc;
}

void CWorkLogic::SetServAddr(const string &strServIp, USHORT uServPort) {
    m_strServIp = strServIp;
    m_uServPort = uServPort;
}

CWorkLogic *CWorkLogic::GetInstance(){
    if (ms_inst == NULL)
    {
        ms_inst = new CWorkLogic();
    }
    return ms_inst;
}

void CWorkLogic::GetJsonPack(Value &v, const string &strDataType)
{
    static int gs_iSerial = 0;
    v["unique"] = m_strDevUnique;
    v["dataType"] = strDataType;
    v["id"] = gs_iSerial++;
}

string CWorkLogic::GetDbgUnique()
{
    return m_strDevUnique;
}

bool CWorkLogic::ConnectDbgClient(const string &strRemote)
{
    try {
        Value vJson;
        GetJsonPack(vJson, CMD_C2S_CONNECT);
        vJson["dst"] = strRemote;
        string strResult;
        SendMsgForResult(vJson.get("id", 0).asUInt(), vJson, strResult, 50000);

        /**
        {
            "stat":0,
            "desc":"abcdef",
            "clientDesc":"client desc"
        }
        */
        Value vResult;
        Reader().parse(strResult, vResult);
        if (vResult.type() != objectValue)
        {
            return false;
        }

        if (vResult["stat"].asInt() != 0)
        {
            return false;
        }
        m_DbgClient.m_strUnique = strRemote;
        m_DbgClient.m_strClientDesc = vResult["clientDesc"].asString();
        m_bConnectSucc = true;
        return true;
    } catch (std::exception &e) {
        MessageBoxA(0, e.what(), "error", 0);
        return false;
    }
}

bool CWorkLogic::IsConnectDbg()
{
    return m_bConnectSucc;
}

ClientInfo CWorkLogic::GetDbgClient()
{
    return m_DbgClient;
}

list<ustring> CWorkLogic::ExecCmd(const ustring &wstrCmd, int iTimeOut)
{
    string strReply;
    Value vRequest;
    GetJsonPack(vRequest, CMD_C2C_RUNCMD);
    vRequest["cmd"] = WtoU(wstrCmd);
    SendToDbgClient(vRequest, strReply, iTimeOut);

    Value vResult;
    Reader().parse(strReply, vResult);

    list<ustring> result;
    if (vResult.type() == arrayValue)
    {
        for (int i = 0 ; i < (int)vResult.size() ; i++)
        {
            result.push_back(UtoW(vResult[i].asString()));
        }

        if (result.empty())
        {
            result.push_back(L"没有任何数据");
        }
    }
    else
    {
        result.push_back(L"等待超时");
    }
    return result;
}

/**获取用户列表**/
string CWorkLogic::RequestClientInternal()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    string strResult;
    SendMsgForResult(vJson.get("id", 0).asUInt(), vJson, strResult, 3000);
    return strResult;
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

//发送命令至被调试端
bool CWorkLogic::SendToDbgClient(Value &vData, string &strResult, int iTimeOut)
{
    int id = vData.get("id", 0).asUInt();
    Value vPacket;
    vPacket["dataType"] = CMD_C2S_TRANSDATA;
    vPacket["src"] = m_strDevUnique;
    vPacket["dst"] = m_DbgClient.m_strUnique;
    vPacket["id"] = 0;
    vPacket["content"] = vData;
    return SendMsgForResult(id, vPacket, strResult, iTimeOut);
}

bool CWorkLogic::SendData(CDbgClient *remote, const Value &vData)
{
    string str = CMD_START_MARK;
    str += FastWriter().write(vData);
    str += CMD_FINISH_MARK;
    remote->SendData(str);
    return true;
}

bool CWorkLogic::SendFtpForResult(int id, Value &vRequest, string &strResult, DWORD dwTimeOut)
{
    return true;
}

bool CWorkLogic::SendMsgForResult(int id, Value &vRequest, string &strResult, DWORD iTimeOut)
{
    HANDLE hNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    RequestInfo *pInfo = new RequestInfo();
    {
        CScopedLocker lock(&m_requsetLock);
        pInfo->m_id = id;
        pInfo->m_hNotify = hNotify;
        pInfo->m_pReply = &strResult;
        m_MsgRequestPool[pInfo->m_id] = pInfo;
        SendData(&m_MsgServ, vRequest);
    }

    DWORD dwResult = WaitForSingleObject(pInfo->m_hNotify, iTimeOut);
    bool bResult = true;
    if (WAIT_TIMEOUT == dwResult)
    {
        CScopedLocker lock(&m_requsetLock);
        m_MsgRequestPool.erase(pInfo->m_id);
        bResult = false;
    }
    CloseHandle(hNotify);
    delete pInfo;

    if (bResult)
    {
        Value vData;
        Reader().parse(strResult, vData);
        Value vContent = vData["content"];
        strResult = FastWriter().write(vContent);
    }
    else
    {
        strResult = "time out";
    }
    return bResult;
}

/**
服务端到终端请求用户列表回执
*/
vector<ClientInfo> CWorkLogic::GetClientSet()
{
    string strReply = RequestClientInternal();

    CScopedLocker lock(&m_clientLock);
    vector<ClientInfo> result;
    Value vClients;
    Reader().parse(strReply, vClients);
    
    for (size_t i = 0 ; i < vClients.size() ; i++)
    {
        Value vSingle = vClients[i];
        ClientInfo info;
        info.m_strUnique = vSingle["unique"].asString();
        info.m_strStartTime = vSingle["startTime"].asString();
        info.m_strClientDesc = vSingle["clientDesc"].asString();
        info.m_strIpInternal = vSingle["ipInternal"].asString();
        info.m_uPortInternal = vSingle["portInternal"].asUInt();
        info.m_strIpExternal = vSingle["ipExternal"].asString();
        info.m_uPortExternal = vSingle["portExternal"].asUInt();
        result.push_back(info);
    }
    return result;
}

void CWorkLogic::OnMsgReply(const string &strData)
{
    Reader reader;
    Value vJson;
    reader.parse(strData, vJson);
    if (vJson.type() != objectValue)
    {
        return;
    }

    string strDataType = vJson.get("dataType", "").asString();
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asUInt();

    CScopedLocker lock(&m_requsetLock);
    map<int, RequestInfo *>::const_iterator it;
    if (m_MsgRequestPool.end() != (it = m_MsgRequestPool.find(id)))
    {
        RequestInfo *ptr = it->second;
        *(ptr->m_pReply) = strData;
        SetEvent(ptr->m_hNotify);
        m_MsgRequestPool.erase(id);
    }
}

void CWorkLogic::OnFtpReply(const string &strData)
{
    Reader reader;
    Value vJson;
    reader.parse(strData, vJson);
    if (vJson.type() != objectValue)
    {
        return;
    }

    string strDataType = vJson.get("dataType", "").asString();
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asUInt();

    CScopedLocker lock(&m_requsetLock);
    map<int, RequestInfo *>::const_iterator it;
    if (m_FtpRequestPool.end() != (it = m_FtpRequestPool.find(id)))
    {
        RequestInfo *ptr = it->second;
        *(ptr->m_pReply) = strData;
        SetEvent(ptr->m_hNotify);
        m_FtpRequestPool.erase(id);
    }
}

//仅仅把外壳剥掉，继续处理
void CWorkLogic::OnMsgTransData(const string &strData)
{
    Reader reader;
    Value vJson;
    reader.parse(strData, vJson);
    if (vJson.type() != objectValue)
    {
        return;
    }

    Value vContent = vJson["content"];
    string strContent = FastWriter().write(vContent);
    OnMsgSingleData(strContent);
}

void CWorkLogic::OnFtpNotify(const string &strData) {
    /**
    {
        "dataType":"ftpFileNotify_s2c",
        "ftpUnique":"abcdef"                //ftp文件标识
    }
    */
    Value vJson;
    Reader().parse(strData, vJson);
    string strFtpUnique = vJson.get("ftpUnique", "").asString();

    /**
    先用现有的长连接请求，以后需要改成短连接
    {
        "dataType":"ftpGetFile",
        "id":1122323,
        "ftpUnique":"abcdef"
    }
    */
    Value vFtpRequest;
    vFtpRequest["ftpUnique"] = strFtpUnique;
    GetJsonPack(vFtpRequest, CMD_FTP_GETFILE);
    SendData(&m_FtpServ, vFtpRequest);
}

void CWorkLogic::OnMsgSingleData(const string &strData)
{
    Reader reader;
    Value vJson;

    reader.parse(strData, vJson);
    if (vJson.type() != objectValue)
    {
        return;
    }

    string strDataType = vJson.get("dataType", "").asString();
    try {
        //数据回执
        if (strDataType == CMD_REPLY)
        {
            OnMsgReply(strData);
        }
        //ftp通知
        else if (strDataType == CMD_C2S_FTPFILE_NOTIFY)
        {
            OnFtpNotify(strData);
        }
        //服务端转发数据
        else if (strDataType == CMD_C2S_TRANSDATA)
        {
            OnMsgTransData(strData);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

wstring CWorkLogic::GetFtpLocalPath(const ustring &wstrDesc, const ustring &wstrUnique, const ustring &wstrFileName, bool bCompress)
{
    //创建临时目录
    WCHAR wszDir[MAX_PATH] = {0};
    GetModuleFileNameW(NULL, wszDir, MAX_PATH);
    PathAppendW(wszDir, L"..\\dbgcache");
    PathAppendW(wszDir, wstrUnique.c_str());
    SHCreateDirectoryExW(NULL, wszDir, NULL);

    SYSTEMTIME time = {0};
    GetLocalTime(&time);

    ustring wstrTmpFile = FormatW(
        L"%04d%02d%02d%02d%02d%02d_%ls",
        time.wYear,
        time.wMonth,
        time.wDay,
        time.wHour,
        time.wMinute,
        time.wSecond,
        wstrFileName.c_str()
        );

    if (bCompress && !wstrTmpFile.endwith(L".zip"))
    {
        wstrTmpFile += L".zip";
    }
    PathAppendW(wszDir, wstrTmpFile.c_str());
    return wszDir;
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
具体的文件内容
*/
void CWorkLogic::OnFileTransferBegin(Value &vJson)
{
    int fileSize = vJson.get("fileSize", 0).asUInt();
    ustring fileName = UtoW(vJson.get("fileName", "").asString());
    if (fileSize <= 0)
    {
        NotifyMessage(L"接收到的文件大小为空");
        return;
    }

    m_FtpCache.m_wstrFileDesc = UtoW(vJson.get("desc", "").asString());
    m_FtpCache.m_wstrFileName = fileName;

    ustring wstrSrc = UtoW(vJson.get("src", "asdf").asString());
    m_FtpCache.m_bCompress = (bool)(vJson.get("compress", true).asBool());
    m_FtpCache.m_wstrLocalPath = GetFtpLocalPath(m_FtpCache.m_wstrFileDesc.c_str(), wstrSrc, m_FtpCache.m_wstrFileName, m_FtpCache.m_bCompress);
    m_FtpCache.m_uRecvSize = 0;
    m_FtpCache.m_uFileSize = fileSize;
    m_FtpCache.m_hTransferFile = CreateFileW(
        m_FtpCache.m_wstrLocalPath.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        CREATE_NEW,
        0,
        NULL
        );
    m_bFtpTransfer = true;
}

void CWorkLogic::OnFtpSingleData(const string &strData)
{
    Reader reader;
    Value vJson;

    reader.parse(strData, vJson);
    if (vJson.type() != objectValue)
    {
        return;
    }

    string strDataType = vJson.get("dataType", "").asString();
    try {
        //数据回执
        if (strDataType == CMD_REPLY)
        {
            OnFtpReply(strData);
        }
        else if (strDataType == CMD_FTP_TRANSFER)
        {
            OnFileTransferBegin(vJson);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
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
void CWorkLogic::OnSendServHeartbeat()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_HEARTBEAT);
    vJson["clientDesc"] = GetDevDesc();
    vJson["ipInternal"] = m_strLocalIp;
    vJson["portInternal"] = m_uLocalPort;
    SendData(&m_MsgServ, vJson);
}

DWORD CWorkLogic::WorkThread(LPVOID pParam)
{
    CWorkLogic *ptr = (CWorkLogic *)pParam;
    while (true)
    {
        ptr->OnSendServHeartbeat();
        Sleep(1000 * 5);
    }
    return 0;
}

void CWorkLogic::OnRecvMsgData(const string &strData)
{
    m_strMsgCache.append(strData);

    size_t lastPos = 0;
    size_t pos = m_strMsgCache.find(CMD_START_MARK);
    if (pos != 0)
    {
        m_strMsgCache.clear();
        return;
    }

    while (true)
    {
        size_t iStart = lstrlenA(CMD_START_MARK);
        pos = m_strMsgCache.find(CMD_FINISH_MARK, iStart);
        if (pos == string::npos)
        {
            break;
        }

        size_t iEnd = pos;
        string strSub = m_strMsgCache.substr(iStart, iEnd - iStart);
        m_strMsgCache.erase(0, iEnd + lstrlenA(CMD_FINISH_MARK));
        GetInstance()->OnMsgSingleData(strSub);
    }
    return;
}

void CWorkLogic::OnFtpTransferStat(string &strData)
{
    ULONGLONG uNeedSize = m_FtpCache.m_uFileSize - m_FtpCache.m_uRecvSize;
    DWORD dwWrited = 0;
    if (uNeedSize >= strData.size())
    {
        m_FtpCache.m_uRecvSize += strData.size();
        WriteFile(m_FtpCache.m_hTransferFile, strData.c_str(), strData.size(), &dwWrited, NULL);
        strData.clear();
    }
    else
    {
        //粘包情况
        m_FtpCache.m_uRecvSize += uNeedSize;
        WriteFile(m_FtpCache.m_hTransferFile, strData.c_str(), (DWORD)uNeedSize, &dwWrited, NULL);
        strData.erase(0, (unsigned int)uNeedSize);
    }

    //文件传输结束
    if (m_FtpCache.m_uFileSize == m_FtpCache.m_uRecvSize)
    {
        CloseHandle(m_FtpCache.m_hTransferFile);
        m_FtpCache.m_hTransferFile = INVALID_HANDLE_VALUE;
        NotifyLogFile(m_FtpCache.m_wstrFileDesc.c_str(), m_FtpCache.m_wstrLocalPath.c_str(), m_FtpCache.m_bCompress);
        m_FtpCache.m_wstrFileDesc.clear();
        m_FtpCache.m_wstrFileName.clear();
        m_FtpCache.m_uFileSize = 0;
        m_FtpCache.m_uRecvSize = 0;
        m_bFtpTransfer = false;
    }
}

void CWorkLogic::OnRecvFtpData(const string &strRecv)
{
    string strData = strRecv;
    //文件内容传输中
    if (m_bFtpTransfer)
    {
        OnFtpTransferStat(strData);
        if (strData.empty())
        {
            return;
        }
    }

    m_strFtpCache.append(strData);

    size_t lastPos = 0;
    size_t pos = m_strFtpCache.find(CMD_START_MARK);
    if (pos != 0)
    {
        m_strFtpCache.clear();
        return;
    }

    while (true)
    {
        size_t iStart = lstrlenA(CMD_START_MARK);
        pos = m_strFtpCache.find(CMD_FINISH_MARK, iStart);
        if (pos == string::npos)
        {
            break;
        }

        size_t iEnd = pos;
        string strSub = m_strFtpCache.substr(iStart, iEnd - iStart);
        m_strFtpCache.erase(0, iEnd + lstrlenA(CMD_FINISH_MARK));
        GetInstance()->OnFtpSingleData(strSub);
    }
    return;
}