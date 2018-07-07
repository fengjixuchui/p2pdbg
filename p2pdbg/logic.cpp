#include <WinSock2.h>
#include <json/json.h>
#include <gdcharconv.h>
#include "logic.h"
#include "cmddef.h"

using namespace Json;

CWorkLogic *CWorkLogic::ms_inst = NULL;

CWorkLogic::CWorkLogic()
{
    m_hCompleteNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    m_bInit = false;
    m_strServIp = IP_SERV;
    m_uServPort = PORT_SERV;
    m_bConnectSucc = false;
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
    m_c2serv.InitClient(IP_SERV, PORT_SERV, 1, this);
    m_strLocalIp = m_c2serv.GetLocalIp();
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

    do
    {
        //���������ⲿ��ַ���ڲ���ַ���г���
        if (m_c2client.InitClient(tmp.m_strIpExternal, tmp.m_uPortExternal, 1, this))
        {
            m_iDbgMode = 0;
            break;
        }

        if (m_c2client.InitClient(tmp.m_strIpInternal, tmp.m_uPortInternal, 1, this))
        {
            m_iDbgMode = 0;
            break;
        }

        //��ʧ�ܵĻ�ͨ���������ת
        m_iDbgMode = 1;
    } while (false);

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

/**��ȡ�û��б�**/
string CWorkLogic::RequestClientInternal()
{
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_GETCLIENTS);
    string strResult;
    SendForResult(&m_c2serv, vJson.get("id", 0).asUInt(), vJson, strResult);
    return strResult;
}

//���������������Զ�
bool CWorkLogic::SendToDbgClient(Value &vData, string &strResult, int iTimeOut)
{
    int id = vData.get("id", 0).asUInt();
    //ֱ�ӷ����Զ�
    if (m_iDbgMode == 0)
    {
        return SendForResult(&m_c2client, id, vData, strResult, iTimeOut);
    }
    //ͨ�������ת�����Զ�
    else if (m_iDbgMode == 1)
    {
        Value vPacket;
        vPacket["dataType"] = CMD_C2S_TRANSDATA;
        vPacket["src"] = m_strDevUnique;
        vPacket["dst"] = m_DbgClient.m_strUnique;
        vPacket["id"] = 0;
        vPacket["content"] = vData;
        return SendForResult(&m_c2serv, id, vPacket, strResult, iTimeOut);
    }
    return false;
}

bool CWorkLogic::SendData(CDbgClient *remote, const string &strData)
{
    string str = CMD_START_MARK;
    str += strData;
    remote->SendData(str);
    return true;
}

bool CWorkLogic::SendForResult(CDbgClient *remote, int id, Value &vRequest, string &strResult, int iTimeOut)
{
    HANDLE hNotify = CreateEventW(NULL, FALSE, FALSE, NULL);
    RequestInfo *pInfo = new RequestInfo();
    {
        CScopedLocker lock(&m_requsetLock);
        pInfo->m_id = id;
        pInfo->m_hNotify = hNotify;
        pInfo->m_pReply = &strResult;
        m_requestPool[pInfo->m_id] = pInfo;
        SendData(remote, FastWriter().write(vRequest));
    }

    DWORD dwResult = WaitForSingleObject(pInfo->m_hNotify, iTimeOut);
    bool bResult = true;
    if (WAIT_TIMEOUT == dwResult)
    {
        CScopedLocker lock(&m_requsetLock);
        m_requestPool.erase(pInfo->m_id);
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
����˵��ն������û��б��ִ
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

void CWorkLogic::OnReply(CDbgClient *ptr, const string &strData)
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
    if (m_requestPool.end() != (it = m_requestPool.find(id)))
    {
        RequestInfo *ptr = it->second;
        *(ptr->m_pReply) = strData;
        SetEvent(ptr->m_hNotify);
        m_requestPool.erase(id);
    }
}

//��������ǰ�������������
void CWorkLogic::OnTransData(CDbgClient *ptr, const string &strData)
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
    OnSingleData(ptr, strContent);
}

void CWorkLogic::OnSingleData(CDbgClient *ptr, const string &strData)
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
        //���ݻ�ִ
        if (strDataType == CMD_REPLY)
        {
            OnReply(ptr, strData);
        }
        else if (strDataType == CMD_FILE_TRANSFER)
        {
        }
        //�����ת������
        else if (strDataType == CMD_C2S_TRANSDATA)
        {
            OnTransData(ptr, strData);
        }
    } catch (std::exception &e) {
        string str = e.what();
    }
}

/**
�ն˵�����˵�����
{
    "dataType":"heartbeat_c2s",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��"

    "clientDesc":"�豸����",
    "ipInternal":"�ڲ�ip",
    "portInternal":"�ڲ��Ķ˿�"
}
*/
void CWorkLogic::OnSendServHeartbeat() {
    Value vJson;
    GetJsonPack(vJson, CMD_C2S_HEARTBEAT);
    vJson["clientDesc"] = GetDevDesc();
    vJson["ipInternal"] = m_strLocalIp;
    vJson["portInternal"] = m_uLocalPort;
    SendData(&m_c2serv, FastWriter().write(vJson));
}

DWORD CWorkLogic::WorkThread(LPVOID pParam)
{
    CWorkLogic *ptr = (CWorkLogic *)pParam;
    while (true) {
        ptr->OnSendServHeartbeat();
        ptr->OnGetClientsInThread();
        Sleep(1000);
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