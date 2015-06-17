/***********************************************************
*文件名：plugin_url.h
*创建人：韩涛
*日  期：2013年10月21日
*描  述：url头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_URL_H_
#define _PLUGIN_URL_H_

#include "plugin_public.h"
#include "plugin_config_macro.h"

//数据截取之后存放到该结构体中，为record的成员变量
struct url_request{
	
	struct plugin_feature *url_feature;				//特征码
	char date[DATE_LENGTH];							//捕获日期
	char time[TIME_LENGTH];							//捕获时间
										
	char account[ACCOUNT_LENGTH + 1];				//登录账户
	char password[PASSWORD_LENGTH + 1];				//登录密码	
	char sessionID[COMMON_LENGTH_512 + 1];			//sessionID
	char url[URL_LENGTH + 1];						//url
	char web[COMMON_LENGTH_512 + 1];				//web

};

//先将数据保存，根据服务端返回结果判断该数据是否处理
struct url_record{
	struct plugin_deal_status plugin_ds;			//处理状态
	
	int url_data_size;								//URL 数据大小
	int thread_num;									//线程号
	char deal_flag;									//处理标志

	struct url_request url_req;						//包含的处理结果结构

	char url_data[URL_STORAGE_LEN + 1];				//临时存储url数组
};


static int __attribute ((constructor)) plugin_init();
static void plugin_feature_init();
static int plugin_deal(const struct data_transit *dtts, const int thread_num);
static int plugin_url_storage(struct url_record *url_rec, const struct data_transit *dtts, const int thread_num);
static int plugin_deal_client(const struct data_transit *dtts, struct url_record *url_rec, const int thread_num);
static int plugin_deal_server(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_web(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_url(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_data_storage(const struct data_transit *dtts, struct url_record *url_dst, const int thread_num);
static void plugin_done(struct url_record *url_dst);

#endif
