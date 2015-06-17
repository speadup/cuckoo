/***********************************************************
*文件名：plugin_search.h
*创建人：韩涛
*日  期：2013年10月21日
*描  述：search头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_SEARCH_H_
#define _PLUGIN_SEARCH_H_

#include "plugin_public.h"
#include "plugin_config_macro.h"

struct search_request{
	struct plugin_deal_status plugin_ds;				//状态结构体

	struct plugin_feature *search_feature;				//特征码
	char date[DATE_LENGTH];								//捕获日期
	char time[TIME_LENGTH];								//捕获时间
	
	char searchword[COMMON_LENGTH_512 + 1];				//搜索关键字
	char url[URL_LENGTH + 1];							//搜索URL
};

static int __attribute ((constructor)) plugin_init();
static void plugin_feature_init();
static int plugin_deal(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num);
static int plugin_deal_searchword(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);
static int plugin_deal_url(char *deal_data, void *dst);
static int plugin_data_storage(const struct data_transit *dtts, struct search_request *search_dst, const int thread_num, const int plugin_id);
static void plugin_done(void *search_dst);

#endif
