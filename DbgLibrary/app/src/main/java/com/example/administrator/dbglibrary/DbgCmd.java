package com.example.administrator.dbglibrary;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Set;

public class DbgCmd {
    public interface DbgCmdHandler {
        LinkedList<String> onDbgCommand(String strCmd, String strParam);
    };

    class DbgCmdInfo {
        String m_strCmd = new String();
        String m_strDesc = new String();
        DbgCmdHandler m_handler = null;
    };

    public synchronized boolean registerCmd(String strCmd, String strDsc, DbgCmdHandler handler) {
        if (m_cmdHandlers.containsKey(strCmd)) {
            return false;
        }

        DbgCmdInfo info = new DbgCmdInfo();
        info.m_strCmd = strCmd;
        info.m_strDesc = strDsc;
        info.m_handler = handler;
        m_cmdHandlers.put(strCmd, info);
        return true;
    }

    public synchronized LinkedList<String> getCmdList() {
        Set<String> keySet = m_cmdHandlers.keySet();
        LinkedList<String> result = new LinkedList<String>();
        for (String strKey : keySet) {
            DbgCmdInfo info = m_cmdHandlers.get(strKey);
            result.addLast(String.format("指令:%s 描述: %s", info.m_strCmd, info.m_strDesc));
        }
        return result;
    }

    public LinkedList<String> runCmd(String strContent) {
        strContent.trim();
        while (strContent.indexOf("  ") >= 0) {
            strContent.replace("  ", " ");
        }

        String strCmd = "";
        String strParam = "";
        int pos = 0;
        if ((pos = strContent.indexOf(" ")) >= 0) {
            strCmd = strContent.substring(0, pos);
            strParam = strContent.substring(pos + 1);
        } else {
            strCmd = strContent;
        }

        LinkedList<String> result = new LinkedList<String>();
        synchronized(this) {
            if (m_cmdHandlers.containsKey(strCmd)) {
                DbgCmdInfo info = m_cmdHandlers.get(strCmd);
                result = info.m_handler.onDbgCommand(strCmd, strParam);
            } else {
                result.addLast("no handler, cmd:" + strContent);
            }
        }
        return result;
    }

    //TODO
    HashMap<String, DbgCmdInfo> m_cmdHandlers = new HashMap<String, DbgCmdInfo>();
};
