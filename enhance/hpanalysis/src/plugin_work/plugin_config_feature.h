/*
************************************************************
*创建人：韩涛
*日  期：2014年2月12日
*描  述：插件写死的特征码 类似配置信息 可修改
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_CONFIG_FEATURE_H_
#define _PLUGIN_CONFIG_FEATURE_H_

//公共
#define PLUGIN_SMTP_TO_ACCOUT	"\r\nTo: "
#define PLUGIN_TAIL "\r\n"
#define PLUGIN_TAIL_TAIL "\r\n\r\n"
#define DECODE_STRING_GB2312 "=?gb2312?B?"
#define DECODE_STRING_GBK "=?gb18030?B?"
#define DECODE_STRING_UTF_8 "=?UTF-8?B?"
#define PLUGIN_HOST "Host: "
#define PLUGIN_GET "GET "
#define PLUGIN_HTTP " HTTP/1.1"


//邮件公共
#define MAIL_ATTACHMENT_HEAD "name=\""
#define MAIL_ATTACHMENT_HEAD_SUB "filename=\""
#define MAIL_ATTACHMENT_TAIL "\"\r\n"

//POP3写死特征码
#define POP3_TAIL "\r\n--"
#define POP3_NEW "retr"
#define POP3_QUIT "quit"
#define MAIL_OK "+OK"
#define MAIL_OCTETS " octets"


//SMTP写死特征码
#define SMTP_TAIL "\r\n\r\n--"
#define SMTP_LOGIN "334"
#define SMTP_NEW "354"
#define SMTP_QUIT "250"


//BBS写死特征码
#define BBS_REFERER_HEAD "Referer: "

//URL写死特征码

#define URL_TYPE_TITLE "Content-Type: "
#define URL_TYPE_HTML "html" //可检测html、shtml、xhtml、dhtml等类型文件
#define URL_TYPE_XML "xml"   //可检测xml类型文件

#define RESPONSE_STATUS_200 "200"
#define RESPONSE_STATUS_304 "304"
#define HTTP_SATTUS_LENGTH 3
#define HTTP_STATUS_INDEX 9 //HTTP/1.1 200 OK
#define HTTP_STATUS_HEAD_LEN 15
#define HTTP_TYPE_TITLE_LENGTH 14


//IM 匹配特征码
//-------------------------------------------------------------
#define QQ_MARK_HEAD_IPHONE "GET /offline/check"
#define QQ_MARK_HEAD_IPHONE_SIZE 18

#define QQ_MARK_FEATURE_IPHONE "offline.qq.com"

#define QQ_MARK_HEAD_ANDROID "stVerifyBlackListReq"
#define QQ_MARK_HEAD_ANDROID_SIZE 20
//--------------------------------------------------------------

#define WEIXIN_MARK_HEAD_IPHONE "POST http://short.weixin.qq.com"
#define WEIXIN_MARK_HEAD_IPHONE_SIZE 31

#define WEIXIN_MARK_FEATURE_IPHONE "getboundharddevices"

#define WEIXIN_MARK_HEAD_ANDROID "POST http://szshort.weixin.qq.com/cgi-bin/micromsg-bin/newauth"
#define WEIXIN_MARK_HEAD_ANDROID_SIZE 62

#define WEIXIN_MARK_FEATURE_ANDROID "szshort.weixin.qq.com"
//---------------------------------------------------------------

#define MOMO_MARK_HEAD_ANDROID "GET /album"
#define MOMO_MARK_HEAD_ANDROID_SIZE 10

#define MOMO_MARK_FEATURE_ANDROID "img.momocdn.com"
//---------------------------------------------------------------

//iphone android都是用iPhone的识别特征码
#define BILIN_MARK_HEAD_IPHONE "GET / HTTP/1.1"
#define BILIN_MARK_HEAD_IPHONE_SIZE 14

#define BILIN_MARK_FEATURE_IPHONE "www.inbilin.com"

//如下特征码是安卓用密码登陆是获取账号
#define BILIN_MARK_HEAD_ANDROID_OTHER "POST /queryUserDetail.html"
#define BILIN_MARK_HEAD_ANDROID_OTHER_SIZE 26

#define BILIN_MARK_FEATURE_ANDROID_OTHER "web20.onbilin.com" 
//--------------------------------------------------------------

//iPhone的识别特征码
#define YIXIN_MARK_HEAD_IPHONE "GET /lbs3"
#define YIXIN_MARK_HEAD_IPHONE_SIZE 9

#define YIXIN_MARK_FEATURE_IPHONE "lbs.dd.163.com"

#define YIXIN_MARK_HEAD_ANDROID "GET /api/queryareacode"
#define YIXIN_MARK_HEAD_ANDROID_SIZE 22

#define YIXIN_MARK_FEATURE_ANDROID "api.dd.163.com"
//---------------------------------------------------------------

#define FETION_MARK_HEAD_IPHONE "POST /mnav/getNetSystemconfig"
#define FETION_MARK_HEAD_ANDROID "POST /mnav/getnetsystemconfig"
#define FETION_MARK_HEAD_IA_SIZE 29

#define FETION_MARK_FEATURE_IA "mnav.fetion.com.cn"

//==============================================================
//IM截取特征码
#define QQ_EXTRACT_HEAD_ACCOUNT_IPHONE "uin=o"  //qq登陆
#define QQ_EXTRACT_TAIL_ACCOUNT_IPHONE ";"

#define QQ_EXTRACT_HEAD_DEVICE_IPHONE "v "     //获取qq使用的设备类型
#define QQ_EXTRACT_TAIL_DEVICE_IPHONE ","

#define QQ_EXTRACT_HEAD_OS_IPHONE "iphone"     //获取qq使用的系统版本
#define QQ_EXTRACT_TAIL_OS_IPHONE "("

#define QQ_EXTRACT_HEAD_ACCOUNT_ANDROID "lv"
#define QQ_EXTRACT_TAIL_ACCOUNT_ANDROID NULL
//-------------------------------------------------------------

#define BILIN_EXTRACT_HEAD_ACCOUNT_ANDROID "userId="  //bilin登陆
#define BILIN_EXTRACT_TAIL_ACCOUNT_ANDROID "&"

#define BILIN_EXTRACT_HEAD_PASSWD_ANDROID "PASSWORD="
#define BILIN_EXTRACT_TAIL_PASSWD_ANDROID "&"
//--------------------------------------------------------------

#define YIXIN_EXTRACT_HEAD_ACCOUNT_ANDROID "mobile="
#define YIXIN_EXTRACT_TAIL_ACCOUNT_ANDROID " "

//--------------------------------------------------------------

#define FETION_EXTRACT_HEAD_ACCOUNT_IA "mobile-no=\""
#define FETION_EXTRACT_TAIL_ACCOUNT_IA "\""


#endif //_PLUGIN_CONFIG_FEATURE_H_

