#ifndef P2P_DEFINE_H_H_
#define P2P_DEFINE_H_H_
/**
Э���ʽJson��ʽ
<Protocol Start>
*/
#define CMD_START_MARK          "<Protocol Start>"

/**�ն˵������ָ��**/
/**
�ն˵�½
{
    "dataType":"login",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��",
    "id":1234

    "clientDesc":"�豸����",
    "ipInternal":"�ڲ�ip",
    "portInternal":"�ڲ��Ķ˿�"
}
*/
#define CMD_C2S_LOGIN           "login_c2s"

/**
�ն˵ǳ�
{
    "dataType":"logout",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��"
}
*/
#define CMD_C2S_LOGOUT          "logout_c2s"

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
#define CMD_C2S_HEARTBEAT       "heartbeat_c2s"

/**
�ն������û��б�
{
    "dataType":"getUserList",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��"
}
*/
#define CMD_C2S_GETCLIENTS      "getUserList_c2s"

/**
�ն�����net��͸
{
    "dataType":"testNetPass_c2s",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��",

    "dstClient":""
}
*/
#define CMD_C2S_TESTNETPASS     "testNetPass_c2s"

/**����˵��ն�ָ��**/
/**
�ն�����net��͸
{
    "dataType":"testNetPass_s2c",
    "time":"����ʱ��",

    "srcUnique":"",
    "srcInternalIp":"",
    "srcInternalPort":1234,
    "srcExternalIp:"",
    "srcExternalPort":1234
}
*/
#define CMD_S2C_TESTNETPASS     "testNetPass_s2c"

/**
����˵��ն�������ִ
{
    "dataType":"heartbeat_s2c",
    "time":"����ʱ��",

    "clientCount":�û�����,
    "clientUpdateTime":�ͻ�����Ϣ������ʱ��
}
*/
#define CMD_S2C_HEARTBEAT      "heartbeat_s2c"

/**
����˵��ն������û��б��ִ
{
    "dataType":"getUserList_s2c",
    "time":"����ʱ��",

    "clients":
    [
        {"unique":"", "clientDesc":"", "ipInternal":"", "portInternal":"", "ipExternal":"", "portExternal":""},
        ...
    ]
}
*/
#define CMD_S2C_GETCLIENTS      "getUserList_s2c"

/**�ն˵��ն�ָ��**/
/**
�ն�����net��͸
{
    "dataType":"testNetPass_c2c",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��",

    "dstClient":""
}
*/
#define CMD_C2C_TESTNETPASS     "testNetPass_c2c"

/**
�ն˵��ն�����
{
    "dataType":"heartbeat_c2c",
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��"
}
*/
#define CMD_C2C_HEARTBEAT       "heartbeat_c2c"
#endif
