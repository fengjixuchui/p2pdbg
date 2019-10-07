package com.example.administrator.dbglibrary;

public class CmdDef {
    /**
     协议格式Json格式
     <Protocol Start>
     */
    static public final String CMD_START_MARK = "<Protocol Start>";
    static public final String CMD_FINISH_MARK  = "<Protocol Finish>";

    /**终端到服务端指令**/
    /**
     终端到服务端的心跳
     {
         "dataType":"heartbeat_c2s",
         "unique":"终端标识",
         "time":"发送时间"

         "clientDesc":"设备描述",
         "ip":"内部ip",
         "port":"内部端口"
     }
     */
    static public final String CMD_C2S_HEARTBEAT = "heartbeat_c2s";

    /**
     终端请求用户列表
     {
         "dataType":"getUserList",
         "unique":"终端标识",
         "time":"发送时间"
     }
     */
    static public final String CMD_C2S_GETCLIENTS = "getUserList_c2s";

    /**服务端到终端指令**/
    /**
     服务器转发其他用户数据
     {
        "dataType":"transdata_s2c",
        "src":"来源终端标识",
        "dst":"目标终端标识",
        "content": {
            "具体的数据内容"
        }
     }
     */
    static public  final String CMD_C2S_TRANSDATA = "transdata_s2c";

    /**终端到终端指令**/
    /**
     控制端请求执行命令
     {
         "dataType":"runcmd_c2c",
         "id":1111,
         "unique":"终端标识",
         "time":"发送时间",

         "cmd":"命令串",
         "content": {
             ""
         }
     }
     */
    static public final String CMD_C2C_RUNCMD = "runcmd_c2c";

    /**
     数据回执，所有数据统一以以下格式回执
     {
         "dataType":"reply",
         "id":111,
         "stat":0,

         "content":{
         ...
        }
     }
     */
     static public final String CMD_REPLY = "reply";

     /**ftp文件传输协议**/
    /**
     文件传输用户注册
     {
         "dataType":"ftpRegister",
         "id":1111,
         "unique":"设备标识"
     }
     */
    static public final String CMD_FTP_REGISTER = "ftpRegister";
    /**
     文件传输
     <Protocol Start>
     {
         "dataType":"ftpTransfer",
         "id":112233,
         "fileUnique":"文件标识",
         "src":"文件发送方",
         "dst":"文件接收方",
         "desc":"文件描述",
         "fileSize":112233,
         "fileName":"文件名"
     }
     <Protocol Finish>
     具体的文件内容
     */
    static public final String CMD_FTP_TRANSFER = "ftpTransfer";
}
