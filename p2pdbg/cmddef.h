#ifndef P2P_DEFINE_H_H_
#define P2P_DEFINE_H_H_

/**
 协议格式Json格式
 <Protocol Start>
 */
#define CMD_START_MARK "<Protocol Start>"
#define CMD_FILE_MARK  "<File Start>"

/**终端到服务端指令**/
/**
 终端到服务端的心跳
 {
     "dataType":"heartbeat_c2s",
     "unique":"终端标识",
     "serial":111,
     "time":"发送时间"

     "clientDesc":"设备描述",
     "ip":"内部ip",
     "port":"内部端口"
 }
 */
#define CMD_C2S_HEARTBEAT "heartbeat_c2s"

/**
 终端请求用户列表
 {
     "dataType":"getUserList",
     "serial":111,
     "unique":"终端标识",
     "time":"发送时间"
 }
 */
#define CMD_C2S_GETCLIENTS "getUserList_c2s"

/**服务端到终端指令**/
/**
 服务器转发其他用户数据
 {
    "dataType":"transdata_s2c",
    "id":1111,
    "src":"来源终端标识",
    "dst":"目标终端标识",

    "content": {
        "具体的数据内容"
    }
 }
 */
#define CMD_C2S_TRANSDATA "transdata_s2c"

/**终端到终端指令**/
/**
 控制端请求执行命令
 {
     "dataType":"runcmd_c2c",
     "unique":"终端标识",
     "time":"发送时间",

     "cmd":"命令标识",
     "content": {
         ""
     }
 }
 */
#define CMD_C2C_RUNCMD "runcmd_c2c"

/**
 * 终端到终端的心跳
 */
#define CMD_C2C_HEARTBEAT "heartbeat_c2c"

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
#define CMD_REPLY   "reply"
#endif
