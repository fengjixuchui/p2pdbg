#include <WinSock2.h>
#include <json/json.h>
#include "logic.h"
#include "cmddef.h"

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = new CWorkLogic();

CWorkLogic::CWorkLogic(){
    m_pUdpServ = new CUdpServ();
    SetServAddr(IP_SERV, PORT_SERV);
    m_pUdpServ->StartServ(PORT_LOCAL, OnRecv);
    m_hP2pNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
}

void CWorkLogic::SetServAddr(const string &strServIp, USHORT uServPort) {
    m_strServIp = strServIp;
    m_uServPort = uServPort;
}

CWorkLogic *CWorkLogic::GetInstance(){
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

        Value vTest;
        GetJsonPack(vTest, CMD_C2C_TESTNETPASS);
        ResetEvent(m_hP2pNotify);
        while (true) {
            if ((GetTickCount() - dwStart) >= dwTimeOut)
            {
                return false;
            }

            Send(FastWriter().write(vTest), info.m_strIpInternal, info.m_uPortInternal);
            Send(FastWriter().write(vTest), info.m_strIpExternal, info.m_uPortExternal);

            if (WaitForSingleObject(m_hP2pNotify, 100) == WAIT_OBJECT_0)
            {
                return true;
            }
        }
    }
    else
    {
        return false;
    }
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

        if (strDataType == CMD_C2S_LOGIN)
        {
            ClientInfo info;
            info.m_strUnique = strUnique;
            info.m_strIpInternal = vJson.get("ipInternal", "").asString();
            info.m_uPortInternal = vJson.get("portInternal", 0).asUInt();
            info.m_strIpExternal = strAddr;
            info.m_uPortExternal = uPort;
            info.m_uLastHeartbeat = GetTickCount();
        } else if (strDataType == CMD_C2S_LOGOUT)
        {
            if (m_clientInfos.end() != m_clientInfos.find(strUnique))
            {
                m_clientInfos.erase(strUnique);
            }
        } else if (strDataType == CMD_C2S_GETCLIENTS)
        {
            Value vResp;
            vResp["id"] = id;
            vResp["status"] = 0;
            Value vClient;

            for (map<string, ClientInfo>::const_iterator it = m_clientInfos.begin() ; it != m_clientInfos.end() ; it++)
            {
                vClient["unique"] = it->second.m_strUnique;
                vClient["ipInternal"] = it->second.m_strIpInternal;
                vClient["portInternal"] = it->second.m_uPortInternal;
                vClient["ipExternal"] = it->second.m_strIpExternal;
                vClient["portExternal"] = it->second.m_uPortExternal;
                vResp["client"].append(vClient);
            }
            Send(FastWriter().write(vResp), strAddr, uPort);
        } else if (strDataType == CMD_C2S_TESTNETPASS)
        {
        } else if (strDataType == CMD_C2C_TESTNETPASS)
        {
            SetEvent(m_hP2pNotify);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

void CWorkLogic::Send(const string &strData, const string &strAddr, USHORT uPort)
{
    m_pUdpServ->SendTo(strAddr, uPort, strData);
}