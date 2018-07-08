#include <WinSock2.h>
#include <json/json.h>
#include <gdcharconv.h>
#include "logic.h"
#include "cmddef.h"

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
    SendMsgForResult(id, vJson, strResult);
    return true;
}

bool CWorkLogic::RegisterFtpServSyn()
{
    Value vJson;
    GetJsonPack(vJson, CMD_FTP_REGISTER);
    int id = vJson["id"].asUInt();
    string strResult;
    SendFtpForResult(id, vJson, strResult);
    return true;
}

void CWorkLogic::StartWork()
{
    if (m_bInit)
    {
        return;
    }

    m_bInit = true;
    srand(GetTickCount());
    m_strDevUnique = fmt(
        "%x%x%x%x%x%x%x%x-%x%x%x%x-%x%x%x%x-%x%x%x%x%x%x%x%x",
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf, 
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf,
        rand() % 0xf, rand() % 0xf, rand() % 0xf, rand() % 0xf
        );
    m_uLocalPort = PORT_LOCAL;
    m_strServIp = GetIpFromDomain(DOMAIN_SERV);
    m_MsgServ.InitClient(m_strServIp, PORT_MSG_SERV, 1, &m_MsgHandler);
    m_FtpServ.InitClient(m_strServIp, PORT_FTP_SERV, 1, &m_FtpHandler);
    m_strLocalIp = m_MsgServ.GetLocalIp();
    RegisterMsgServSyn();
    RegisterFtpServSyn();
    m_hWorkThread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
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

bool CWorkLogic::ConnectReomte(const string &strRemote)
{
    ClientInfo tmp;
    {
        CScopedLocker lock(&m_clientLock);
        map<string, ClientInfo>::const_iterator it = m_clientInfos.find(strRemote);
        if (it == m_clientInfos.end())
        {
            return false;
        }
        tmp = it->second;
    }

    m_bConnectSucc = true;
    m_DbgClient = tmp;
    return true;
}

bool CWorkLogic::IsConnectDbg()
{
    return m_bConnectSucc;
}

ClientInfo CWorkLogic::GetDbgClient()
{
    return m_DbgClient;
}

wstring CWorkLogic::ExecCmd(const wstring &wstrCmd, int iTimeOut)
{
    string strReply;
    Value vRequest;
    GetJsonPack(vRequest, CMD_C2C_RUNCMD);
    vRequest["cmd"] = WtoU(wstrCmd);
    SendToDbgClient(vRequest, strReply, iTimeOut);

    Value vReply;
    Reader().parse(strReply, vReply);
    return UtoW(StyledWriter().write(vReply));
}

vector<ClientInfo> CWorkLogic::GetClientList()
{
    CScopedLocker lock(&m_clientLock);
    vector<ClientInfo> vResult;
    for (map<string, ClientInfo>::const_iterator it = m_clientInfos.begin() ; it != m_clientInfos.end() ; it++)
    {
        vResult.push_back(it->second);
    }
    return vResult;
}

/**获取用户列表**/
string CWorkLogic::RequestClientInternal()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    string strResult;
    SendMsgForResult(vJson.get("id", 0).asUInt(), vJson, strResult);
    return strResult;
}

string CWorkLogic::GetIpFromDomain(const string &strDomain)
{
    HOSTENT *host_entry = gethostbyname(strDomain.c_str());
    if(host_entry != NULL)
    {
        return fmt(
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

bool CWorkLogic::SendFtpForResult(int id, Value &vRequest, string &strResult)
{
    HANDLE hNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    RequestInfo *pInfo = new RequestInfo();
    {
        CScopedLocker lock(&m_requsetLock);
        pInfo->m_id = id;
        pInfo->m_hNotify = hNotify;
        pInfo->m_pReply = &strResult;
        m_FtpRequestPool[pInfo->m_id] = pInfo;
        SendData(&m_FtpServ, vRequest);
    }

    DWORD dwResult = WaitForSingleObject(pInfo->m_hNotify, INFINITE);
    bool bResult = true;
    if (WAIT_TIMEOUT == dwResult)
    {
        CScopedLocker lock(&m_requsetLock);
        m_FtpRequestPool.erase(pInfo->m_id);
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
        return "cmd timeout";
    }
    return true;
}

bool CWorkLogic::SendMsgForResult(int id, Value &vRequest, string &strResult, int iTimeOut)
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
        return "cmd timeout";
    }
    return true;
}

/**
服务端到终端请求用户列表回执
*/
void CWorkLogic::OnGetClientsInThread()
{
    string strReply = RequestClientInternal();

    CScopedLocker lock(&m_clientLock);
    m_clientInfos.clear();
    Value vClients;
    Reader().parse(strReply, vClients);
    
    for (size_t i = 0 ; i < vClients.size() ; i++)
    {
        Value vSingle = vClients[i];
        ClientInfo info;
        info.m_strUnique = vSingle["unique"].asString();
        info.m_strClientDesc = vSingle["clientDesc"].asString();
        info.m_strIpInternal = vSingle["ipInternal"].asString();
        info.m_uPortInternal = vSingle["portInternal"].asUInt();
        info.m_strIpExternal = vSingle["ipExternal"].asString();
        info.m_uPortExternal = vSingle["portExternal"].asUInt();
        m_clientInfos[info.m_strUnique] = info;
    }
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
        //服务端转发数据
        else if (strDataType == CMD_C2S_TRANSDATA)
        {
            OnMsgTransData(strData);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
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
    SendData(&m_MsgServ, FastWriter().write(vJson));
}

DWORD CWorkLogic::WorkThread(LPVOID pParam)
{
    CWorkLogic *ptr = (CWorkLogic *)pParam;
    while (true)
    {
        ptr->OnSendServHeartbeat();
        ptr->OnGetClientsInThread();
        Sleep(1000);
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

void CWorkLogic::OnRecvFtpData(const string &strData)
{
    m_strFtpCache.append(strData);

    size_t lastPos = 0;
    size_t pos = m_strFtpCache.find(CMD_START_MARK);
    if (pos != 0)
    {
        m_strMsgCache.clear();
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