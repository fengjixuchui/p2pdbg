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
}

void CWorkLogic::StartWork() {
    if (m_bInit)
    {
        return;
    }

    m_bInit = true;
    m_pUdpServ->StartServ(PORT_LOCAL, OnRecv);
    m_uLocalPort = PORT_LOCAL;
    m_strLocalIp = m_pUdpServ->GetLocalIp();
    RegisterUser();
    m_hWorkThread = CreateThread(NULL, 0, WorkThread, this, 0, NULL);
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

        Value vTest;
        GetJsonPack(vTest, CMD_C2C_TESTNETPASS);
        ResetEvent(m_hP2pNotify);
        while (true) {
            if ((GetTickCount() - dwStart) >= dwTimeOut)
            {
                return false;
            }

            SendTo(info.m_strIpInternal, info.m_uPortInternal, FastWriter().write(vTest));
            SendTo(info.m_strIpExternal, info.m_uPortExternal, FastWriter().write(vTest));

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
void CWorkLogic::GetUserList(){
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    SendTo(m_strServIp, m_uServPort, FastWriter().write(vJson));
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
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

void CWorkLogic::OnSendServHeartbeat() {
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_HEARTBEAT);
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
        ptr->GetUserList();
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