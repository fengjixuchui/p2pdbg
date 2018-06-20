#include <WinSock2.h>
#include <json/json.h>
#include "logic.h"
#include "cmddef.h"

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = NULL;

CWorkLogic::CWorkLogic(){
    m_pUdpServ = new CUdpServ();
    m_hP2pNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_bP2pStat = false;
    m_bInit = false;
    m_strServIp = IP_SERV;
    m_uServPort = PORT_SERV;
    m_pHandler = NULL;
}

void CWorkLogic::StartWork(CDataHandler *pHandler) {
    if (m_bInit)
    {
        return;
    }

    m_bInit = true;
    m_pUdpServ->StartServ(PORT_LOCAL, OnRecv);
    m_uLocalPort = PORT_LOCAL;
    m_strLocalIp = m_pUdpServ->GetLocalIp();
    m_pHandler = pHandler;
    RegisterUser();
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

void CWorkLogic::GetJsonPack(Value &v, const string &strDataType) {
    static int gs_iSerial = 0;
    v["unique"] = UNIQUE;
    v["dataType"] = strDataType;
    v["id"] = gs_iSerial++;
}

bool CWorkLogic::Connect(const string &strUnique, DWORD dwTimeOut){
    if (m_clientInfos.end() != m_clientInfos.find(strUnique))
    {
        DWORD dwStart = GetTickCount();
        ClientInfo info = m_clientInfos[strUnique];

        Value vToServ;
        GetJsonPack(vToServ, CMD_C2S_TESTNETPASS);
        vToServ["dstClient"] = strUnique;

        Value vToClient;
        GetJsonPack(vToClient, CMD_C2C_TESTNETPASS);

        ResetEvent(m_hP2pNotify);
        while (true) {
            if ((GetTickCount() - dwStart) >= dwTimeOut)
            {
                return false;
            }

            SendTo(m_strServIp, m_uServPort, FastWriter().write(vToServ));
            SendTo(info.m_strIpInternal, info.m_uPortInternal, FastWriter().write(vToClient));
            SendTo(info.m_strIpExternal, info.m_uPortExternal, FastWriter().write(vToClient));

            if (WaitForSingleObject(m_hP2pNotify, 100) == WAIT_OBJECT_0)
            {
                m_bP2pStat = true;
                return true;
            }
        }
    }
    else
    {
        return false;
    }
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

void CWorkLogic::OnRecv(const string &strData, const string &strAddr, USHORT uPort)
{
    size_t lastPos = 0;
    size_t pos = strData.find(PACKET_STARTMARK);
    if (pos != 0)
    {
        return;
    }

    while (true) {
        pos = strData.find(PACKET_STARTMARK, pos + lstrlenA(PACKET_STARTMARK));
        if (pos == string::npos)
        {
            break;
        }

        lastPos += lstrlenA(PACKET_STARTMARK);
        string strSub = strData.substr(lastPos, pos - lastPos);
        GetInstance()->OnSingleData(strSub, strAddr, uPort);
        lastPos = pos;
    }

    if (lastPos != strData.length())
    {
        lastPos += lstrlenA(PACKET_STARTMARK);
        string strSub = strData.substr(lastPos, strData.size() - lastPos);
        GetInstance()->OnSingleData(strSub, strAddr, uPort);
    }
}

/**注册用户**/
/**
"clientDesc":"设备描述",
"ipInternal":"内部ip",
"portInternal":"内部的端口"
*/
void CWorkLogic::RegisterUser(){
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_LOGIN);
    vJson["clientDesc"] = DBGDESC;
    vJson["ipInternal"] = m_strLocalIp;
    vJson["portInternal"] = m_uLocalPort;
    SendTo(m_strServIp, m_uServPort, FastWriter().write(vJson));
}

/**获取用户列表**/
void CWorkLogic::RequestClients(){
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    SendTo(m_strServIp, m_uServPort, FastWriter().write(vJson));
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

void CWorkLogic::OnSingleData(const string &strData, const string &strAddr, USHORT uPort)
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

        if (strDataType == CMD_S2C_TESTNETPASS)
        {
        } else if (strDataType == CMD_C2C_TESTNETPASS)
        {
            SetEvent(m_hP2pNotify);
        } else if (strDataType == CMD_S2C_GETCLIENTS)
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
    SendTo(m_strServIp, m_uServPort, FastWriter().write(vJson));
}

void CWorkLogic::OnSendP2pHearbeat() {
    if (m_bP2pStat)
    {
        Value vJson;
        GetJsonPack(vJson, CMD_C2C_HEARTBEAT);
        SendTo(m_strP2pIp, m_uP2pPort, FastWriter().write(vJson));
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

void CWorkLogic::SendTo(const string &strAddr, USHORT uPort, const string &strData)
{
    string str = CMD_START_MARK;
    str += strData;
    m_pUdpServ->SendTo(strAddr, uPort, str);
}