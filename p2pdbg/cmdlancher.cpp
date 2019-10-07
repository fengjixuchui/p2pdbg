#include <WinSock2.h>
#include <Windows.h>
#include "../ComLib/StrUtil.h"
#include "cmdlancher.h"
#include "logic.h"

CCmdLancher *CCmdLancher::GetInstance()
{
    static CCmdLancher *s_inst = NULL;
    if (s_inst == NULL)
    {
        s_inst = new CCmdLancher();
    }
    return s_inst;
}

list<ustring> CCmdLancher::GetLocalCmdList()
{
    CScopedLocker lock(this);
    list<ustring> result;
    for (list<CmdLocalInfo *>::const_iterator it = m_cmdList.begin() ; it != m_cmdList.end() ; it++)
    {
        CmdLocalInfo *ptr = *it;
        result.push_back(FormatW(L"ÃüÁî:%ls ÃèÊö:%ls", ptr->m_wstrCmd.c_str(), ptr->m_wstrDesc.c_str()));
    }
    return result;
}

list<ustring> CCmdLancher::GetRemoteCmdList()
{
    if (!CWorkLogic::GetInstance()->IsConnectDbg())
    {
        return list<ustring>();
    }

    return CWorkLogic::GetInstance()->ExecCmd("cmd", 5000);
}

list<ustring> CCmdLancher::RunCmd(const ustring &wstrCmd)
{
    ustring wstr1;
    ustring wstr2;
    ustring wstrTmp = wstrCmd;
    wstrTmp.trim();

    size_t pos = -1;
    while (wstrTmp.find(L"  ") != ustring::npos) {
        wstrTmp.repsub(L"  ", L" ");
    }

    if ((pos = wstrTmp.find(L" ")) != ustring::npos)
    {
        wstr1 = wstrTmp.substr(0, pos);
        wstr2 = wstrTmp.substr(pos + 1, wstrTmp.size() - pos - 1);
    }
    else {
        wstr1 = wstrTmp;
    }

    list<ustring> result;
    CmdLocalInfo *pLocalCmd = GetLocalCmd(wstr1);

    //±¾µØÃüÁî
    if (pLocalCmd)
    {
        return pLocalCmd->m_pfnProc(wstr1, wstr2);
    }

    if (!CWorkLogic::GetInstance()->IsConnectDbg())
    {
        if (!pLocalCmd)
        {
            result.push_back(FormatW(L"²»Ö§³ÖµÄÃüÁî:%ls", wstr1.c_str()));
        }
        return result;
    }

    //Ô¶¶ËÃüÁî
    return CWorkLogic::GetInstance()->ExecCmd(wstrTmp, 5000);
}

bool CCmdLancher::RegisterCmd(const ustring &wstrCmd, const ustring &wstrDesc, pfnCmdProc proc)
{
    if (NULL != GetLocalCmd(wstrCmd))
    {
        return false;
    }

    CmdLocalInfo *ptr = new CmdLocalInfo();
    ptr->m_wstrCmd = wstrCmd;
    ptr->m_wstrDesc = wstrDesc;
    ptr->m_pfnProc = proc;
    CScopedLocker lock(this);
    m_cmdList.push_back(ptr);
    return true;
}

CmdLocalInfo *CCmdLancher::GetLocalCmd(const ustring &wstrCmd) {
    CScopedLocker lock(this);
    ustring tmp = wstrCmd;
    tmp.trim();
    for (list<CmdLocalInfo *>::const_iterator it = m_cmdList.begin() ; it != m_cmdList.end() ; it++)
    {
        CmdLocalInfo *ij = *it;
        if (0 == ij->m_wstrCmd.comparei(tmp))
        {
            return ij;
        }
    }
    return NULL;
}