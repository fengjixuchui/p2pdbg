#include <WinSock2.h>
#include <json/json.h>
#include "logic.h"
#include "cmddef.h"

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = NULL;

CWorkLogic::CWorkLogic(){
    m_pUdpServ = NULL;
    m_bInit = false;
    m_hWorkThread = NULL;
}

bool CWorkLogic::StartWork(USHORT uLocalPort)
{
    if (m_bInit)
    {
        return true;
    }

    m_bInit = true;
    m_pUdpServ = new CUdpServ();
    m_pUdpServ->StartServ(uLocalPort, OnRecv);
    m_hWorkThread = CreateThread(NULL, 0, ServWorkThread, this, 0, NULL);
    return true;
}

CWorkLogic *CWorkLogic::GetInstance(){
    if (ms_inst == NULL)
    {
        ms_inst = new CWorkLogic();
    }
    return ms_inst;
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

void CWorkLogic::OnClientLogin(const string &strAddr, USHORT uPort, const Value &vJson)
{
    try {
        if (vJson.type() != objectValue)
        {
            return;
        }

        string strDataType = vJson.get("dataType", "").asString();
        string strUnique = vJson.get("unique", "").asString();
        int id = vJson.get("id", 0).asUInt();

        ClientInfo info;
        info.m_strUnique = strUnique;
        info.m_strClientDesc = vJson.get("clientDesc", "").asString();
        info.m_strIpInternal = vJson.get("ipInternal", "").asString();
        info.m_uPortInternal = vJson.get("portInternal", 0).asUInt();
        info.m_strIpExternal = strAddr;
        info.m_uPortExternal = uPort;
        info.m_uLastHeartbeat = GetTickCount();

        m_clientInfos[strUnique] = info;
    } catch (exception &e)
    {
        e.what();
    }
}

void CWorkLogic::OnClientLogout(const string &strAddr, USHORT uPort, const Value &vJson)
{
    string strDataType = vJson.get("dataType", "").asString();
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asUInt();

    if (m_clientInfos.end() != m_clientInfos.find(strUnique))
    {
        m_clientInfos.erase(strUnique);
    }
}

void CWorkLogic::OnGetClient(const string &strAddr, USHORT uPort, const Value &vJson)
{
    string strDataType = vJson.get("dataType", "").asString();
    string strUnique = vJson.get("unique", "").asString();
    int id = vJson.get("id", 0).asUInt();

    Value vResp;
    vResp["dataType"] = CMD_S2C_GETCLIENTS;
    vResp["id"] = id;
    vResp["status"] = 0;
    Value vClient;
    Value vArry(arrayValue);

    for (map<string, ClientInfo>::const_iterator it = m_clientInfos.begin() ; it != m_clientInfos.end() ; it++)
    {
        vClient["unique"] = it->second.m_strUnique;
        vClient["clientDesc"] = it->second.m_strClientDesc;
        vClient["ipInternal"] = it->second.m_strIpInternal;
        vClient["portInternal"] = it->second.m_uPortInternal;
        vClient["ipExternal"] = it->second.m_strIpExternal;
        vClient["portExternal"] = it->second.m_uPortExternal;
        vArry.append(vClient);
    }
    vResp["clients"] = vArry;
    SendTo(strAddr, uPort, FastWriter().write(vResp));
}

/**
终端请求net穿透
{
    "dataType":"testNetPass_s2c",
    "time":"发送时间",

    "srcUnique":"",
    "srcInternalIp":"",
    "srcInternalPort":1234,
    "srcExternalIp:"",
    "srcExternalPort":1234
}
*/
//将穿透请求推送给目标终端
void CWorkLogic::OnRequestNetPass(const string &strIp, USHORT uPort, const Value &vJson)
{
    string strDst = vJson.get("dstClient", "").asString();
    if (m_clientInfos.end() == m_clientInfos.find(strDst))
    {
        return;
    }

    string strSrcUnique = vJson.get("unique", "").asString();
    ClientInfo dstInfo = m_clientInfos[strDst];
    ClientInfo srcInfo = m_clientInfos[strSrcUnique];

    Value vPacket;
    vPacket["dataType"] = CMD_S2C_TESTNETPASS;
    vPacket["srcUnique"] = strSrcUnique;
    vPacket["srcInternalIp"] = srcInfo.m_strIpInternal;
    vPacket["srcInternalPort"] = srcInfo.m_uPortInternal;
    vPacket["srcExternalIp"] = srcInfo.m_strIpExternal;
    vPacket["srcExternalPort"] = srcInfo.m_uPortExternal;
    SendTo(dstInfo.m_strIpExternal, dstInfo.m_uPortExternal, FastWriter().write(vPacket));
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
void CWorkLogic::OnClientHeartbeat(const string &strIp, USHORT uPort, const Value &vJson)
{
    string strUnique = vJson.get("unique", "").asString();
    string strInternalIp = vJson.get("ipInternal", "").asString();
    int uInternalPort = vJson.get("portInternal", 0).asUInt();
    string strClientDesc = vJson.get("clientDesc", "").asString();

    map<string, ClientInfo>::iterator it;
    if (m_clientInfos.end() != (it = m_clientInfos.find(strUnique)))
    {
        it->second.m_uLastHeartbeat = GetTickCount();
    }
    else
    {
        ClientInfo info;
        info.m_strUnique = strUnique;
        info.m_strClientDesc = strClientDesc;
        info.m_strIpInternal = strInternalIp;
        info.m_uPortInternal = uInternalPort;
        info.m_strIpExternal = strIp;
        info.m_uPortExternal = uPort;
        info.m_uLastHeartbeat = GetTickCount();
        m_clientInfos[strUnique] = info;
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

        if (strDataType == CMD_C2S_LOGIN)
        {
            OnClientLogin(strAddr, uPort, vJson);
        } else if (strDataType == CMD_C2S_LOGOUT)
        {
            OnClientLogout(strAddr, uPort, vJson);
        } else if (strDataType == CMD_C2S_GETCLIENTS)
        {
            OnGetClient(strAddr, uPort, vJson);
        } else if (strDataType == CMD_C2S_TESTNETPASS)
        {
            OnRequestNetPass(strAddr, uPort, vJson);
        } else if (strDataType == CMD_C2S_HEARTBEAT)
        {
            OnClientHeartbeat(strAddr, uPort, vJson);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

void CWorkLogic::OnCheckClientStat() {
    CScopedLocker lock(this);
    DWORD dwCurCount = GetTickCount();
    for (map<string, ClientInfo>::const_iterator it = m_clientInfos.begin() ; it != m_clientInfos.end() ;)
    {
        if ((dwCurCount - it->second.m_uLastHeartbeat) >= TIMECOUNT_MAX)
        {
            it = m_clientInfos.erase(it);
            continue;
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

string CWorkLogic::GetLocalIp(){
    return m_pUdpServ->GetLocalIp();
}

void CWorkLogic::SendTo(const string &strAddr, USHORT uPort, const string &strData)
{
    string str = PACKET_STARTMARK;
    str += strData;
    m_pUdpServ->SendTo(strAddr, uPort, str);
}