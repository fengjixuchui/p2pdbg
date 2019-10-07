package com.example.administrator.dbglibrary;

import android.content.Context;
import android.os.SystemClock;
import android.util.Log;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.File;
import java.io.FileInputStream;
import java.net.InetAddress;
import java.net.Socket;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.LinkedList;

public class DbgLogic {
    private static void printLog(String strContent) {
        Log.e("DbgLogic", strContent);
    }

    static class MsgDataHandler implements DbgServ.DbgServHandler {
        public void onConnect(Socket sock) {
            ms_strLocalIp = ms_dbgServ.getLocalIp();
            registerMsgServSyn();
        }

        public void onRecvData(Socket sock, byte userData[]) {
            ms_strMsgCache += new String(userData);
            if (!ms_strMsgCache.startsWith(CmdDef.CMD_START_MARK)) {
                ms_strMsgCache = "";
                return;
            }

            while (true) {
                int iPos = ms_strMsgCache.indexOf(CmdDef.CMD_FINISH_MARK, CmdDef.CMD_START_MARK.length());

                if (iPos < 0) {
                    break;
                }
                onRecvMsgSingleData(sock, ms_strMsgCache.substring(CmdDef.CMD_START_MARK.length(), iPos));
                ms_strMsgCache = ms_strMsgCache.substring(iPos + CmdDef.CMD_FINISH_MARK.length());
            }
        }
    };

    static class FtpDataHandler implements DbgServ.DbgServHandler {
        public void onConnect(Socket sock) {
        }

        public void onRecvData(Socket sock, byte userData[]) {
            ms_strFtpCache += new String(userData);
            if (!ms_strFtpCache.startsWith(CmdDef.CMD_START_MARK)) {
                ms_strFtpCache = "";
                return;
            }

            while (true) {
                int iPos = ms_strFtpCache.indexOf(CmdDef.CMD_FINISH_MARK, CmdDef.CMD_START_MARK.length());

                if (iPos < 0) {
                    break;
                }
                onRecvFtpSingleData(sock, ms_strFtpCache.substring(CmdDef.CMD_START_MARK.length(), iPos));
                ms_strFtpCache = ms_strFtpCache.substring(iPos + CmdDef.CMD_FINISH_MARK.length());
            }
        }
    };

    private static JSONObject onRunCommand(Socket sock, JSONObject jsonData) {
        JSONArray arry = new JSONArray();
        LinkedList<String> result = new LinkedList<String>();
        try {
            ms_iLastActivityCount = System.currentTimeMillis();
            int id = jsonData.getInt("id");
            result = m_cmd.runCmd(jsonData.getString("cmd"));
            for (String str : result) {
                arry.put(str);
            }

            return makeReply(id, 0, arry);
        } catch (Exception e) {
            return null;
        }
    }

    private static JSONObject onServTransData(Socket sock, JSONObject jsonData) {
        try {
            //TODO 暂时先通过转发数据提取调试器标识，不太严谨，将来要加入调试注册机制进行注册
            ms_strDbgUnique = jsonData.getString("src");
            JSONObject jsonContent = jsonData.getJSONObject("content");
            return onRunCommand(sock, jsonContent);
        } catch (Exception e) {
        }
        return null;
    }

    private static void onMsgServReply(Socket sock, JSONObject jsonObj) {
        try {
            int id = jsonObj.getInt("id");
            synchronized (ms_msgRequestPool) {
                if (ms_msgRequestPool.containsKey(id)) {
                    RequestTask task = ms_msgRequestPool.get(id);
                    task.m_result = jsonObj.getJSONObject("content");
                    synchronized (task.m_notify) {
                        task.m_notify.notify();
                    }
                }
            }
        } catch (Exception e) {
        }
    }

    private static void onRecvMsgSingleData(Socket sock, String strData) {
        try {
            JSONObject jsonObj = new JSONObject(strData);
            String strDataType = jsonObj.getString("dataType");
            int id = jsonObj.getInt("id");

            JSONObject reply = null;
            if (strDataType.contentEquals(CmdDef.CMD_C2C_RUNCMD)) {
                //TODO 控制端执行命令
                reply = onRunCommand(sock, jsonObj);
            } else if (strDataType.contentEquals(CmdDef.CMD_C2S_TRANSDATA)) {
                //TODO 服务端转发的命令
                reply = onServTransData(sock, jsonObj);
            } else  if (strDataType.contentEquals(CmdDef.CMD_REPLY)) {
                //TODO 服务端返回的回执
                onMsgServReply(sock, jsonObj);
            }
            else {
                reply = makeReply(id, 1, new JSONObject());
            }

            if (reply != null) {
                sendToDbgger(reply);
            }
        } catch (Exception e) {
        }
    }

    private static void onFtpServReply(Socket sock, JSONObject jsonObj) {
        try {
            int id = jsonObj.getInt("id");
            synchronized (ms_ftpRequestPool) {
                if (ms_ftpRequestPool.containsKey(id)) {
                    RequestTask task = ms_ftpRequestPool.get(id);
                    task.m_result = jsonObj.getJSONObject("content");
                    synchronized (task.m_notify) {
                        task.m_notify.notify();
                    }
                }
            }
        } catch (Exception e) {
        }
    }

    private static void onRecvFtpSingleData(Socket sock, String strData) {
        try {
            JSONObject jsonObj = new JSONObject(strData);
            String strDataType = jsonObj.getString("dataType");
            int id = jsonObj.getInt("id");
            if (strDataType.contentEquals(CmdDef.CMD_REPLY)) {
                onFtpServReply(sock, jsonObj);
            }
        } catch (Exception e) {
        }
    }

    /**
     * 与服务端的心跳维持
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
    private static void onSendHeartbeatToServ() {
        try {
            JSONObject jsonObj = getBaseJson(CmdDef.CMD_C2S_HEARTBEAT);
            jsonObj.put("clientDesc", getDevDesc());
            jsonObj.put("ipInternal", ms_strLocalIp);
            jsonObj.put("portInternal", PORT_LOCAL);

            sendToServ(jsonObj);
        } catch (Exception e) {
        }
    }

    private static void onCheckActivityStat() {
        long curCount = System.currentTimeMillis();

        if (curCount - ms_iLastActivityCount > ms_iMaxActivityCount) {
            new Thread() {
                public void run() {
                    DbgLogic.stopDbgServ();
                }
            }.start();
        }
    }

    static class DbgLogicWorkThread extends Thread {
        public void run() {
            while (true) {
                synchronized (ms_stopNotify) {
                    try {
                        ms_stopNotify.wait(5000);
                    } catch (Exception e) {
                    }

                    if (ms_bStopWork) {
                        break;
                    }
                }

                onSendHeartbeatToServ();
                onCheckActivityStat();
            }
        }
    };

    private static JSONObject makeReply(int id, int iStat, Object content) {
        try {
            JSONObject result = new JSONObject();
            result.put("dataType", CmdDef.CMD_REPLY);
            result.put("id", id);
            result.put("stat", iStat);
            result.put("content", content);
            return result;
        } catch (Exception e) {
        }
        return null;
    }

    /**
     * 投递给消息服务器并等待结果返回
     */
    private static JSONObject sendToMsgServForResult(JSONObject jsonSend) {
        try {
            RequestTask request = new RequestTask();
            request.m_id = jsonSend.getInt("id");
            synchronized (ms_msgRequestPool) {
                ms_msgRequestPool.put(request.m_id, request);
            }

            sendToServ(jsonSend);
            synchronized (request.m_notify) {
                request.m_notify.wait(5000);
            }

            synchronized (ms_msgRequestPool) {
                ms_msgRequestPool.remove(request.m_id);
            }
            return request.m_result;
        } catch (Exception e) {
            return  null;
        }
    }

    /**
     * 同步方式注册调试终端
     */
    private static boolean registerMsgServSyn() {
        try {
            JSONObject jsonObj = getBaseJson(CmdDef.CMD_C2S_HEARTBEAT);
            jsonObj.put("clientDesc", getDevDesc());
            jsonObj.put("ipInternal", ms_strLocalIp);
            jsonObj.put("portInternal", PORT_LOCAL);
            sendToMsgServForResult(jsonObj);
        } catch (Exception e) {
        }
        return true;
    }

    private static boolean testConnentServ() {
        ms_dbgServ.initDbgServ(IP_SERV, PORT_MSG_SERV, PORT_FTP_SERV, PORT_LOCAL, new MsgDataHandler(), new FtpDataHandler());
        ms_strLocalIp = ms_dbgServ.getLocalIp();
        return true;
    }

    public static boolean stopDbgServ() {
        if (!ms_bStart) {
            return true;
        }

        try {
            ms_bStart = false;
            ms_dbgServ.stopDbgServ();

            ms_bStopWork = true;
            synchronized (ms_stopNotify) {
                ms_stopNotify.notifyAll();
            }
            ms_hWorkThread.join(5000);
        } catch (Exception e) {
        }
        return true;
    }

    public static boolean startDbgServ() {
        if (ms_bStart) {
            return true;
        }

        ms_bStart = true;
        ms_iLastActivityCount = System.currentTimeMillis();
        ms_bStopWork = false;
        IP_SERV = getIpFromDomain(DOMAIN_SERV);
        ms_dbgServ = new DbgServ();

        testConnentServ();
        ms_hWorkThread = new DbgLogicWorkThread();
        ms_hWorkThread.start();
        return true;
    }

    public static boolean isDbgServInit() {
        return ms_bInit;
    }

    public static boolean initDbgServ(String strUnique, Context ctx) {
        if (ms_bInit) {
            return true;
        }

        ms_bInit = true;
        ms_bStopWork = false;
        ms_strDevUnique = strUnique;
        ms_strDevDesc = getDevDesc();
        ms_strWorkDir = ctx.getFilesDir() + "/DbgLib";
        new File(ms_strWorkDir).mkdirs();

        registerCmd("cmd", "命令列表", new DbgCmd.DbgCmdHandler() {
            @Override
            public LinkedList<String> onDbgCommand(String strCmd, String strParam) {
                return m_cmd.getCmdList();
            }
        });

        registerCmd("restart", "重启调试", new DbgCmd.DbgCmdHandler() {
            @Override
            public LinkedList<String> onDbgCommand(String strCmd, String strParam) {
                LinkedList<String> result = new LinkedList<String>();
                result.addLast("正在重启调试功能");
                new Thread() {
                    public void run() {
                        DbgLogic.stopDbgServ();
                        DbgLogic.startDbgServ();
                    }
                }.start();
                return result;
            }
        });
        return true;
    }

    public static String getUnique() {
        return ms_strDevUnique;
    }

    public static boolean registerCmd(String strCmd, String strDesc, DbgCmd.DbgCmdHandler handler) {
        return m_cmd.registerCmd(strCmd, strDesc, handler);
    }

    private static String getZipPath() {
        String strPath = ms_strWorkDir + "/";

        SimpleDateFormat sdf =   new SimpleDateFormat( "yyyyMMddHHmmssSSS" );
        String strTimeStr = sdf.format(new Date());
        strPath += getDevDesc();
        strPath += String.format("%s_%s.zip", getDevDesc(), strTimeStr);
        return strPath;
    }

    //TODO 发送文件或者文件夹到目标
    public static boolean sendFileToDbgger(File file, String strDesc) {
        JSONObject jsonObj = new JSONObject();
        String strZipPath = "";
        Socket ftpSock = ms_dbgServ.getFtpClient();

        if (ftpSock == null) {
            return false;
        }

        try {
            if (!file.exists()) {
                printLog("err1");
                return false;
            }

            int iCompress = 0;
            File sendFile = null;
            String strFileName = file.getName();
            if (file.isDirectory()) {
                strZipPath = getZipPath();
                printLog("zip:" + strZipPath);
                DbgCompress.zipFile(file, strZipPath);
                sendFile = new File(strZipPath);
                iCompress = 1;
            } else {
                sendFile = file;
            }

            jsonObj = getBaseJson(CmdDef.CMD_FTP_TRANSFER);
            jsonObj.put("src", ms_strDevUnique);
            jsonObj.put("dst", ms_strDbgUnique);
            jsonObj.put("desc", strDesc);
            jsonObj.put("fileSize", sendFile.length());
            jsonObj.put("fileName", strFileName);
            jsonObj.put("compress", iCompress);

            String strData = (CmdDef.CMD_START_MARK + jsonObj.toString() + CmdDef.CMD_FINISH_MARK);
            ms_dbgServ.send(ftpSock, strData.getBytes());

            printLog("ftp start");

            byte buffer[] = new byte[4096];
            int iReadSize = 0;
            FileInputStream fileIn = new FileInputStream(sendFile);
            while ((iReadSize = fileIn.read(buffer, 0, 4096)) > 0) {
                byte sendBuffer[] = new byte[iReadSize];
                System.arraycopy(buffer, 0, sendBuffer, 0, iReadSize);
                ms_dbgServ.send(ftpSock, sendBuffer);
                printLog("send data");
            }
        } catch (Exception e) {
            printLog("err:" + e.getMessage());
        } finally {
            try {
                if (strZipPath.length() > 0) {
                    printLog("删除zip文件:" + strZipPath);
                    new File(strZipPath).delete();
                }

                if (ftpSock != null && !ftpSock.isClosed()) {
                    ftpSock.close();
                }
            } catch (Exception e) {
            }
        }
        return true;
    }

    static JSONObject getBaseJson(String strDataType) {
        try {
            JSONObject json = new JSONObject();
            json.put("unique", ms_strDevUnique);
            json.put("id", ms_iSerial++);
            json.put("dataType", strDataType);
            return json;
        } catch (Exception e) {
            return null;
        }
    }

    static protected String getDevDesc() {
        return (android.os.Build.MODEL + "|" +  android.os.Build.MANUFACTURER + "|" + android.os.Build.VERSION.SDK_INT);
    }

    private static boolean sendToServ(JSONObject jsonSend) {
        String str = (CmdDef.CMD_START_MARK + jsonSend.toString() + CmdDef.CMD_FINISH_MARK);
        return ms_dbgServ.sendToServ(str.getBytes());
    }

    /**
     * 路由数据到调试器
     */
    private static boolean sendToDbgger(JSONObject content) {
        try {
            JSONObject jsonObj = new JSONObject();
            //TODO 颠倒接受者和发送者，将数据路由回去
            jsonObj.put("content", content);
            jsonObj.put("src", ms_strDevUnique);
            jsonObj.put("dst", ms_strDbgUnique);
            jsonObj.put("id", 0);
            jsonObj.put("dataType", CmdDef.CMD_C2S_TRANSDATA);
            return sendToServ(jsonObj);
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * 根据域名获取ip地址
     */
    private static String getIpFromDomain(String strDomain) {
        try {
            InetAddress[] inetAddressArr = InetAddress.getAllByName(strDomain);
            if (inetAddressArr != null && inetAddressArr.length > 0) {
                return inetAddressArr[0].getHostAddress();
            }
        } catch (Exception e) {
            return null;
        }
        return null;
    }

    //TODO 调试器私有数据声明
    private static String IP_SERV = "10.10.16.38";
    private static final String DOMAIN_SERV = "p2pdbg.picp.io";
    private static final int PORT_MSG_SERV = 9971;
    private static final int PORT_FTP_SERV = 9981;
    private static final int PORT_LOCAL = 8871;
    private static String ms_strDbgUnique = null;
    private static String ms_strLocalIp = "";
    private static String ms_strDevDesc = "test";
    private static String ms_strWorkDir = null;
    private static int ms_iSerial = 0;

    private static String ms_strDevUnique = null;
    private static DbgServ ms_dbgServ = null;
    private static boolean ms_bInit = false;
    private static boolean ms_bStart = false;
    private static DbgCmd m_cmd = new DbgCmd();
    private static String ms_strMsgCache = new String();
    private static String ms_strFtpCache = new String();
    private static Thread ms_hWorkThread = null;
    private static Object ms_stopNotify = new Object();
    private static boolean ms_bStopWork = false;

    private static long ms_iLastActivityCount = 0;
    private static long ms_iMaxActivityCount = 1000 * 60 * 60;

    /**
     * 请求结果池，用于返回对应请求的结果
     */
    static class RequestTask {
        Object m_notify = new Object();
        int m_id = 0;
        JSONObject m_result = null;
    };
    private static HashMap<Integer, RequestTask> ms_msgRequestPool = new HashMap<Integer, RequestTask>();
    private static HashMap<Integer, RequestTask> ms_ftpRequestPool = new HashMap<Integer, RequestTask>();
}
