#ifndef P2P_DEFINE_H_H_
#define P2P_DEFINE_H_H_
/**
协议格式Json格式
<Protocol Start>
*/

/**终端到服务端指令**/
/**
终端登陆
{
    "dataType":"login",
    "unique":"终端标识",
    "time":"发送时间",
    "id":1234

    "clientDesc":"设备描述",
    "ipInternal":"内部ip",
    "portInternal":"内部的端口"
}
*/
#define CMD_C2S_LOGIN           "login_c2s"

/**
终端登出
{
    "dataType":"logout",
    "unique":"终端标识",
    "time":"发送时间"
}
*/
#define CMD_C2S_LOGOUT          "logout_c2s"

/**
终端到服务端的心跳
{
    "dataType":"heartbeat_c2s",
    "unique":"终端标识",
    "time":"发送时间"
}
*/
#define CMD_C2S_HEARTBEAT       "heartbeat_c2s"

/**
终端请求用户列表
{
    "dataType":"getUserList",
    "unique":"终端标识",
    "time":"发送时间"
}
*/
#define CMD_C2S_GETCLIENTS      "getUserList_c2s"

/**
终端请求net穿透
{
    "dataType":"testNetPass_c2s",
    "unique":"终端标识",
    "time":"发送时间",

    "dstClient":""
}
*/
#define CMD_C2S_TESTNETPASS     "testNetPass_c2s"

/**服务端到终端指令**/
/**
终端请求net穿透
{
    "dataType":"testNetPass_s2c",
    "unique":"终端标识",
    "time":"发送时间",

    "dstClient":""
}
*/
#define CMD_S2C_TESTNETPASS     "testNetPass_s2c"

/**终端到终端指令**/
/**
终端请求net穿透
{
    "dataType":"testNetPass_c2c",
    "unique":"终端标识",
    "time":"发送时间",

    "dstClient":""
}
*/
#define CMD_C2C_TESTNETPASS     "testNetPass_c2c"
#endif
