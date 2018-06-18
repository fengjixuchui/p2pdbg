#ifndef P2P_DEFINE_H_H_
#define P2P_DEFINE_H_H_
/**
Э���ʽJson��ʽ
<Protocol Start>
*/

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
    "unique":"�ն˱�ʶ",
    "time":"����ʱ��",

    "dstClient":""
}
*/
#define CMD_S2C_TESTNETPASS     "testNetPass_s2c"

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
#endif
