#ifndef P2PDBG_CCMDLANCHER
#define P2PDBG_CCMDLANCHER
#include <Windows.h>
#include <list>
#include "../ComLib/mstring.h"
#include "../ComLib/LockBase.h"

using namespace std;

typedef list<ustring> (*pfnCmdProc)(const ustring &wstrCmd, const ustring &wstrParam);

struct CmdLocalInfo {
    ustring m_wstrCmd;          //ÃüÁîÃû³Æ
    ustring m_wstrDesc;         //ÃüÁîÃèÊö
    pfnCmdProc m_pfnProc;       //ÃüÁî»Øµ÷
};

class CCmdLancher : public CCriticalSectionLockable
{
private:
    CCmdLancher() {}

public:
    static CCmdLancher *GetInstance();
    list<ustring> GetLocalCmdList();
    list<ustring> GetRemoteCmdList();
    list<ustring> RunCmd(const ustring &wstrCmd);
    bool RegisterCmd(const ustring &wstrCmd, const ustring &wstrDesc, pfnCmdProc proc);

protected:
    CmdLocalInfo *GetLocalCmd(const ustring &wstrCmd);

protected:
    list<CmdLocalInfo *> m_cmdList;
};
#endif