/***********************************************************
*文件名：feature_manager.c
*创建人：韩涛
*日  期：2013年10月17日
*描  述：特征码管理模块的相关接口实现
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#include "feature_manager.h"
#include "feature_manager_deal.h"
#include "feature_manager_place.h"
#include "feature_manager_public_struct.h"

#include "list.h"
#include "log.h"
#include "stdyeetec.h"

/*
*函数名称：feature_manager_init
*函数功能：特征码管理模块初始化
*输入参数：无
*输出参数：无
*返回值：  0成功；-1错误
*/
int feature_manager_init()
{
	INFO_PRINT("\n----->Feature manager模块初始化..........\n");
	
	//计数器
	global_shared.retain_count++;
	
	//特征码处理初始化
	if(feature_manager_deal_init()){
		log_write(LOG_WARNING, "feature_manager__deal() error or no feature\n");
		return YT_FAILED;
	}
	//place信息初始化
	if(feature_manager_place_init()){
		log_write(LOG_ERROR, "feature_manager_place_init() error\n");
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_manager_get_feature
*函数功能：为插件提供的特征码获取函数
*输入参数：plugin_group_id：插件groupid
*输出参数：plugin_feature_data：特征码链表头
*返回值：  0成功 -1失败
		 	
*/
int feature_manager_get_feature(struct plugin_feature *plugin_feature_data, int plugin_group_id)
{
	int row_num = 0;
	int i = 0;

	//获取头
	struct plugin_feature *plugin_feature_head = feature_manager_deal_get(&row_num);
	if(!plugin_feature_head){
		log_write(LOG_ERROR, "feature_manager_get() error or no feature\n");
		return YT_FAILED;
	}
	
	//遍历整个链，找到相应的结构体,添加到链表中
	while(i < row_num){
		if(plugin_group_id == plugin_feature_head[i].groupid){
			list_add(&(plugin_feature_head[i].list_node), &(plugin_feature_data->list_node));
		}
		i++;
	}
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_manager_destory
*函数功能：特征码管理模块释放函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void feature_manager_destroy()
{
	INFO_PRINT("\n----->Feature manager模块释放..........\n");
	
	//计数器
	global_shared.retain_count--;
	
	feature_manager_deal_destroy();
	feature_manager_place_destory();
}
