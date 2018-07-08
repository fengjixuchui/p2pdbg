#ifndef P2P_DEFINE_H_H_
#define P2P_DEFINE_H_H_

/**
 Э���ʽJson��ʽ
 <Protocol Start>
 */
#define CMD_START_MARK      "<Protocol Start>"
#define CMD_FINISH_MARK     "<Protocol Finish>"

/**�ն˵������ָ��**/
/**
 �ն˵�����˵�����
 {
     "dataType":"heartbeat_c2s",
     "unique":"�ն˱�ʶ",
     "serial":111,
     "time":"����ʱ��"

     "clientDesc":"�豸����",
     "ip":"�ڲ�ip",
     "port":"�ڲ��˿�"
 }
 */
#define CMD_C2S_HEARTBEAT "heartbeat_c2s"

/**
 �ն������û��б�
 {
     "dataType":"getUserList",
     "serial":111,
     "unique":"�ն˱�ʶ",
     "time":"����ʱ��"
 }
 */
#define CMD_C2S_GETCLIENTS "getUserList_c2s"

/**����˵��ն�ָ��**/
/**
 ������ת�������û�����
 {
    "dataType":"transdata_s2c",
    "id":1111,
    "src":"��Դ�ն˱�ʶ",
    "dst":"Ŀ���ն˱�ʶ",

    "content": {
        "�������������"
    }
 }
 */
#define CMD_C2S_TRANSDATA "transdata_s2c"

/**�ն˵��ն�ָ��**/
/**
 ���ƶ�����ִ������
 {
     "dataType":"runcmd_c2c",
     "unique":"�ն˱�ʶ",
     "time":"����ʱ��",

     "cmd":"�����ʶ",
     "content": {
         ""
     }
 }
 */
#define CMD_C2C_RUNCMD "runcmd_c2c"

/**
 * �ն˵��ն˵�����
 */
#define CMD_C2C_HEARTBEAT "heartbeat_c2c"

/**
���ݻ�ִ����������ͳһ�����¸�ʽ��ִ
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

/**�ļ�����Э��**/
/**
�ļ������û�ע��
{
    "dataType":"ftpRegister",
    "id":1111,
    "unique":"�豸��ʶ"
}
*/
#define CMD_FTP_REGISTER    "ftpRegister"

/**
<Protocol Start>
{
    "dataType":"ftpTransfer",
    "id":112233,
    "fileUnique":"�ļ���ʶ",
    "src":"�ļ����ͷ�",
    "dst":"�ļ����շ�",
    "desc":"�ļ�����",
    "fileSize":112233,
    "fileName":"�ļ���"
}
<Protocol Finish>
������ļ�����
*/
#define CMD_FTP_TRANSFER   "ftpTransfer"
#endif
