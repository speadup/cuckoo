/***********************************************************
*文件名：plugin_mail_sina.h
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：sina头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_GAME_H_
#define _PLUGIN_GAME_H_

#include "plugin_public.h"
#include "plugin_config_macro.h"

//先将数据保存，根据服务端返回结果判断该数据是否处理
struct sina_record{
    struct plugin_deal_status plugin_ds;

	char date[DATE_LENGTH];							//捕获日期
	char time[TIME_LENGTH];							//捕获时间
										
	char account[ACCOUNT_LENGTH + 1];				//登录账户
	char password[PASSWORD_LENGTH+1];
	char device_type[COMMON_LENGTH_16];
	char os_ver[COMMON_LENGTH_16];
	int  action;
};

static int __attribute ((constructor)) plugin_init();
static int plugin_deal(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_client(const struct data_transit *dtts, struct sina_record *sina_rec, const int thread_num);
static int plugin_data_storage(const struct data_transit *dtts, struct sina_record *sina_dst, const int thread_num);
static void plugin_done(struct sina_record *sina_dst);

#endif
