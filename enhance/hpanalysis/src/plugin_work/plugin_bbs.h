/***********************************************************
*文件名：plugin_bbs.h
*创建人：韩涛
*日  期：2013年10月21日
*描  述：bbs头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_BBS_H_
#define _PLUGIN_BBS_H_

#include "plugin_config_macro.h"
#include "plugin_public.h"

struct bbs_request{
	struct plugin_deal_status plugin_ds;
	
	char title[TITLE_LENGTH + 1];						//帖子标题
	char content[BBS_CONTENT_LENGTH + 1];				//帖子正文
	char attachment_name[ATTACHMENT_NAME_LENGTH + 1];	//帖子附件名
	int  attachment_size;								//帖子附件大小
	char attachment_path[COMMON_LENGTH_512 + 1];		//帖子附件存放路径

	char subbbs[COMMON_LENGTH_512 + 1];					//帖子板块
	char account[ACCOUNT_LENGTH + 1];					//登录账户
	char password[PASSWORD_LENGTH + 1];					//登录密码	
	char sessionID[COMMON_LENGTH_512 + 1];				//sessionID
	char refererurl[URL_LENGTH + 1];					//refererurl
	char post_address[COMMON_LENGTH_512 + 1];			//发帖地址
	
	struct plugin_feature *bbs_feature;					//特征码
	char date[DATE_LENGTH];								//捕获日期
	char time[TIME_LENGTH];								//捕获时间
	int bbs_attachment_flag;							//附件标志

	int deal_flag;										//是否匹配过一次了

	int bbs_data_size;								    //BBS 数据大小
	int thread_num;									    //线程号
	char bbs_data[COMMON_LENGTH_4096 + 1];				//临时存储url数组
};

static int __attribute ((constructor)) plugin_init();
static int plugin_deal(const struct data_transit *dtts, const int thread_num);
static void plugin_feature_init();
static int plugin_bbs_storage(struct bbs_request *bbs_req, const struct data_transit *dtts, const int thread_num);
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_title(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_refererurl(char *deal_data, void *dst);
static int plugin_deal_subbbs(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_data_storage(const struct data_transit *dtts, struct bbs_request *bbs_dst, const int thread_num, const int plugin_id);
static void plugin_done(void *bbs_dst);
#endif
