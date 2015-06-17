/***********************************************************
*文件名：feature_manager.h
*创建人：韩涛
*日  期：2013年10月17日
*描  述：对外提供特征码管理模块的相关接口
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _FEATURE_MANAGER_H_
#define _FEATURE_MANAGER_H_

#include "feature_manager_public_struct.h"

extern const char place_info[2][256];

/*
*函数名称：feature_manager_init
*函数功能：feature_manager_init模块初始化函数
*输入参数：无
*输出参数：无
*返回值：0 成功 -1失败
*/
int feature_manager_init();

/*
*函数名称：feature_manager_get_feature
*函数功能：为插件提供的特征码获取函数
*输入参数：plugin_group_id:插件troupid
*输出参数:plugin_feature_data：链表头
*返回值：0表示成功。	
		-1表示错误。
*/
int feature_manager_get_feature(struct plugin_feature *plugin_feature_data, int plugin_group_id);

/*
*函数名称：feature_manager_destory
*函数功能：特征码管理模块释放函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void feature_manager_destroy();

#endif
