/*
************************************************************
*创建人：韩涛
*日  期：2013年10月21日
*描  述：插件写死的宏 类似配置信息 可修改
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _PLUGIN_CONFIG_MACRO_H_
#define _PLUGIN_CONFIG_MACRO_H_

//数据长度
#define MAC_LENGTH 16
#define IP_LENGTH 33 //IPV4 15+1=16, IPV6 32+1=33
#define ACCOUNT_LENGTH 64
#define PASSWORD_LENGTH 64
#define TITLE_LENGTH 1024 
#define MAIL_CONTENT_LENGTH 1024 * 4
#define BBS_CONTENT_LENGTH 1024 * 1024 * 4
#define ATTACHMENT_NAME_LENGTH 1024
#define URL_LENGTH 512
#define DATE_LENGTH 16
#define TIME_LENGTH 16
#define URL_STORAGE_LEN 1024

//通用长度
#define COMMON_LENGTH_16 16
#define COMMON_LENGTH_32 32
#define COMMON_LENGTH_64 64
#define COMMON_LENGTH_128 128
#define COMMON_LENGTH_256 256
#define COMMON_LENGTH_512 512
#define COMMON_LENGTH_1024 1024
#define COMMON_LENGTH_2048 2048
#define COMMON_LENGTH_4096	4096


//80 8080端口会话类型
#define PLUGIN_TYPE_SEARCH 1 
#define PLUGIN_TYPE_BBS 2
#define PLUGIN_TYPE_URL 3
#define PLUGIN_TYPE_MAIL_139  400
#define PLUGIN_TYPE_IM_FETION 500
#define PLUGIN_TYPE_IM_WEBQQ  501

//插件是否处理完成标志，减少后续包的处理
#define PLUGIN_DEAL_OVER 1

//插件ID及端口定义
#define POP3_GROUP_ID 21
#define POP3_PORT_NUM 1
#define POP3_PORT 110

#define SMTP_GROUP_ID 31
#define SMTP_PORT_NUM 1
#define SMTP_PORT 25 

#define BBS_GROUP_ID 41
#define BBS_PORT_NUM 2

#define SEARCH_GROUP_ID 51
#define SEARCH_PORT_NUM 2

#define URL_GROUP_ID 11
#define URL_PORT_NUM 2

#define WEB_MAIL_GROUP_ID 61

#define IM_GROUP_ID 71

#define GAME_GROUP_ID 81
#define GAME_PORT_NUM 1

#define IMAP_GROUP_ID 91
#define IMAP_PORT_NUM 1
#define IMAP_PORT 143

#define STOCK_GROUP_ID 15
#define STOCK_PORT_NUM 1

#define FTP_GROUP_ID 25
#define FTP_PORT_NUM 1

#define PROXY_GROUP_ID 35
#define PROXY_PORT_NUM 1

//插件标号 传递给存储模块用于拼接文件 切勿随意改动
#define PLUGIN_MAIL_ID 0
#define PLUGIN_BBS_ID 1
#define PLUGIN_SEARCH_ID 2
#define PLUGIN_URL_ID 3

//客户端或服务端
#define CLIENT_FEATURE 0
#define SERVER_FEATURE 1

//数据是否处理
#define DATA_NO_DEAL 0
#define DATA_DEAL 1

//拼接字符串入文件时buf大小
#define STORAGE_BUF_LEN 1024 * 10

//邮件字段
#define PLUGIN_ACCOUNT "faccount" //对应faccount字段，数组中位置0
#define PLUGIN_ACCOUNT_NUM 0
#define PLUGIN_PASSWORD "fpassword" //对应fpassword字段，数组中位置1
#define PLUGIN_PASSWORD_NUM 1
#define PLUGIN_FROMACCOUNT "ffromaccount" //对应ffromaccount字段，数组中国位置2
#define PLUGIN_FROMACCOUNT_NUM 2
#define PLUGIN_TOACCOUNT "ftoaccount" //对应ftoaccount字段，数组中位置3
#define PLUGIN_TOACCOUNT_NUM 3
#define PLUGIN_CCACCOUNT "fccaccount" //对应fccaccount字段，数组中位置4
#define PLUGIN_CCACCOUNT_NUM 4
#define PLUGIN_TITLE "ftitle" //对应ftitle字段，数组中位置5
#define PLUGIN_TITLE_NUM 5
#define PLUGIN_CONTENT "fcontent" //对应fcontent字段，数组中位置6
#define PLUGIN_CONTENT_NUM 6
#define PLUGIN_ATTACHMENT "fattachment" //对应fattachmentname字段，数组中位置7
#define PLUGIN_ATTACHMENT_NUM 7
#define PLUGIN_MAIL_SERVER "fmailserver" //对应fmailserver字段，数组位置为8
#define PLUGIN_MAIL_SERVER_NUM 8
#define PLUGIN_QUIT "fquit" //对应quit字段，数组位置魏9
#define PLUGIN_QUIT_NUM 9

//80 8080插件字段及对应的数组中位置
#define PLUGIN_SEARCHWORD "fsearchword"
#define PLUGIN_URL "furl"
#define PLUGIN_WEB "fweb"
#define PLUGIN_SUBBBS "fsubbbs"
#define BBS_TITLE_NUM 2
#define BBS_SUBBBS_NUM 4 
#define BBS_CONTENT_NUM 3
#define SEARCH_SEARCHWORD_NUM 0
#define URL_WEB_NUM 2 
#define URL_URL_NUM 3

//特征码字段数组的大小
#define MAIL_FEATURE_FIELD_NUM 10
#define BBS_FEATURE_FIELD_NUM 5 
#define SEARCH_FEATURE_FIELD_NUM 1 
#define URL_FEATURE_FIELD_NUM 4


//建表的时候表前缀
#define URL_PRIFIX    "t_url_log_"
#define BBS_PRIFIX    "t_bbs_log_"
#define MAIL_PRIFIX   "t_mail_log_"
#define SEARCH_PRIFIX "t_search_log_"
#define IM_PRIFIX     "t_im_log_"
#define GAME_PRIFIX   "t_game_log_"
#define STOCK_PRIFIX  "t_stock_log_"
#define FTP_PRIFIX    "t_ftp_log_"
#define PROXY_PRIFIX  "t_proxy_log_"

//字段分隔符 
#define FD_SPE "<yeetecfield>"

//行分割符
#define FD_HEAD "<yeetecline>"

#define PLACECODE "37020221000000"
#define PLACENAME "1111"


#endif
