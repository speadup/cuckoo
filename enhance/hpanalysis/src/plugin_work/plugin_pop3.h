/***********************************************************
*文件名：plugin_pop3.h
*创建人：韩涛
*日  期：2013年10月21日
*描  述：pop3头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_POP3_H_
#define _PLUGIN_POP3_H_

#include "plugin_config_macro.h"
#include "plugin_public.h"

struct pop3_data_deal{
	char *pop3_data;			//存储待截取邮件数据
	char account_deal_flag;		//用户名处理标志
	char password_deal_flag;	//密码处理标志
	char userinfo_done_flag;	//登录信息处理标志
	char quit_deal_flag;		//退出标志
	char from_deal_flag;		//发送人处理标志
	char to_deal_flag;			//收件人处理标志
	char cc_deal_flag;			//抄送人处理标志
	char title_deal_flag;		//标题处理标志
	char content_deal_head;		//因为邮件内容可能很长，不像其他的每次都一起遍历的话，很耗，
	char content_deal_tail;		//所以先找到头，然后在接下来的数据中找尾，其他的可能头尾一起遍历。
	char *eml_content_head;		//临时存储邮件内容头
	char *eml_content_tail;		//临时存储邮件内容尾
	char *attachment_head;		//临时存储附件头
	char content_deal_flag;		//内容处理标志
	char server_deal_flag;		//mailserver处理标志
	char attachment_deal_flag;	//附件处理标志
};
 
struct mail_pop3
{
	int retr_flag;											//客户端拉取邮件标志
	int attachment_flag;									//附件标识符
	
	int eml_size;											//邮件大小
	int eml_real_size;										//包中解析的实际大小
	FILE *mail_file;										//邮件存储句柄，省的每次都要打开，存储数据时要关闭
	char eml_localpath[COMMON_LENGTH_256 + 1];				//邮件存放地址
	
	char account[ACCOUNT_LENGTH + 1];						//登录账户
	char password[PASSWORD_LENGTH + 1];						//登录密码	
	char from[COMMON_LENGTH_64 + 1];						//发件人
	char to[COMMON_LENGTH_1024 + 1];						//收件人
	char cc[COMMON_LENGTH_1024 + 1];						//抄送
	char title[COMMON_LENGTH_512 + 1];						//邮件标题
	char eml_content[MAIL_CONTENT_LENGTH + 1];				//邮件正文
	char eml_attachment_name[ATTACHMENT_NAME_LENGTH + 1];   //附件名称
	char mail_server[COMMON_LENGTH_128 + 1];				//邮件服务器
	
	char date[DATE_LENGTH + 1];								//数据捕获日期
	char time[TIME_LENGTH + 1];								//数据捕获时间
	
	struct plugin_feature *pop3_feature;					//处理该动作时对应的特征码结构
	struct pop3_data_deal deal_flag;

	unsigned char  client_or_server;						//client or server data ,use FROM_*_DATA.
};

static int __attribute ((constructor))plugin_init();
static void plugin_feature_init(void);
static int plugin_deal(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_server(const struct data_transit *dtts, struct mail_pop3 *pop3_dst, const int thread_num);
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_quit(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_storage_mail(char *deal_data, int data_len, struct mail_pop3 *pop3_dst, const int thread_num);
static int plugin_deal_from(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_to(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_cc(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_title(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_real_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_attachment(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_mail_server(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_data_storage(const struct data_transit *dtts, struct mail_pop3 *pop3_dst, const int thread_num, const int plugin_id);
static void plugin_done(struct mail_pop3 *pop3_dst);

#endif
