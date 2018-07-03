#include <WinSock2.h>
#include <json/json.h>
#include "logic.h"
#include "cmddef.h"

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = NULL;

CWorkLogic::CWorkLogic()
{
    m_hP2pNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_hCompleteNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_bP2pStat = false;
    m_bInit = false;
    m_strServIp = IP_SERV;
    m_uServPort = PORT_SERV;
}

void CWorkLogic::StartWork()
{
    if (m_bInit)
    {
        return;
    }
    m_bInit = true;
    m_uLocalPort = PORT_LOCAL;
    m_c2serv.InitClient(IP_SERV, PORT_SERV, this);
    RequestClients();
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
    v["unique"] = UNIQUE;
    v["dataType"] = strDataType;
    v["id"] = gs_iSerial++;
}

vector<ClientInfo> CWorkLogic::GetClientList()
{
    vector<ClientInfo> vResult;
    for (map<string, ClientInfo>::const_iterator it = m_clientInfos.begin() ; it != m_clientInfos.end() ; it++)
    {
        vResult.push_back(it->second);
    }
    return vResult;
}

/**获取用户列表**/
void CWorkLogic::RequestClients()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    string strResult;
    SendForResult(&m_c2serv, vJson, strResult);
}

bool CWorkLogic::SendData(CDbgClient *remote, const string &strData)
{
    string str = CMD_START_MARK;
    str += strData;
    remote->SendData(str);
    return true;
}

bool CWorkLogic::SendForResult(CDbgClient *remote, Value &vRequest, string &strResult)
{
    HANDLE hNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    RequestInfo *pInfo = new RequestInfo();
    {
        CScopedLocker lock(&m_requsetLock);
        pInfo->m_id = vRequest.get("id", 0).asUInt();
        pInfo->m_hNotify = hNotify;
        pInfo->m_pReply = &strResult;
        m_requestPool[pInfo->m_id] = pInfo;
        SendData(remote, FastWriter().write(vRequest));
    }

    WaitForSingleObject(pInfo->m_hNotify, INFINITE);
    CloseHandle(hNotify);

    Value vData;
    Reader().parse(strResult, vData);
    Value vContent = vData["content"];
    strResult = FastWriter().write(vContent);
    return true;
}

/**
服务端到终端请求用户列表回执
{
    "dataType":"getUserList_s2c",
    "time":"发送时间",

    "clients":
    [
        {"unique":"", "clientDesc":"", "ipInternal":"", "portInternal":"", "ipExternal":"", "portExternal":""},
        ...
    ]
}
*/
void CWorkLogic::OnGetClients(const Value &vJson)
{
    m_clientInfos.clear();
    Value vClients = vJson["clients"];
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

void CWorkLogic::OnSingleData(CDbgClient *ptr, const string &strData)
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
        string strUnique = vJson.get("unique", "").asString();
        int id = vJson.get("id", 0).asUInt();

        if (strDataType == CMD_REPLY)
        {
            CScopedLocker lock(&m_requsetLock);
            map<int, RequestInfo *>::const_iterator it;
            if (m_requestPool.end() != (it = m_requestPool.find(id)))
            {
                RequestInfo *ptr = it->second;
                *(ptr->m_pReply) = strData;
                SetEvent(ptr->m_hNotify);
                m_requestPool.erase(id);
            }
        }
        else if (strDataType == CMD_C2S_GETCLIENTS)
        {
            OnGetClients(vJson);
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
void CWorkLogic::OnSendServHeartbeat() {
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_HEARTBEAT);
    vJson["clientDesc"] = GetDevDesc();
    vJson["ipInternal"] = m_strLocalIp;
    vJson["portInternal"] = m_uLocalPort;
    SendTo(FastWriter().write(vJson));
}

void CWorkLogic::OnSendP2pHearbeat() {
    if (m_bP2pStat)
    {
        Value vJson;
        GetJsonPack(vJson, CMD_C2C_HEARTBEAT);
        SendTo(FastWriter().write(vJson));
    }
}

DWORD CWorkLogic::WorkThread(LPVOID pParam)
{
    CWorkLogic *ptr = (CWorkLogic *)pParam;
    while (true) {
        Sleep(1000);
        ptr->OnSendServHeartbeat();
        ptr->OnSendP2pHearbeat();
    }
    return 0;
}

void CWorkLogic::onRecvData(CDbgClient *ptr, const string &strData)
{
    size_t lastPos = 0;
    size_t pos = strData.find(CMD_START_MARK);
    if (pos != 0)
    {
        return;
    }

    while (true) {
        pos = strData.find(CMD_START_MARK, pos + lstrlenA(CMD_START_MARK));
        if (pos == string::npos)
        {
            break;
        }

        lastPos += lstrlenA(CMD_START_MARK);
        string strSub = strData.substr(lastPos, pos - lastPos);
        GetInstance()->OnSingleData(ptr, strSub);
        lastPos = pos;
    }

    if (lastPos != strData.length())
    {
        lastPos += lstrlenA(CMD_START_MARK);
        string strSub = strData.substr(lastPos, strData.size() - lastPos);
        GetInstance()->OnSingleData(ptr, strSub);
    }
}

void CWorkLogic::SendTo(const string &strData)
{
}