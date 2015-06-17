/***********************************************************
*文件名：storage_unit.c
*创建人：韩涛
*日  期：2013年11月5日
*描  述：存储模块C文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#include "storage_unit.h"
#include "storage_unit_database.h"
#include "storage_unit_timer.h"
#include "stdyeetec.h"

/*
*函数名称：storage_unit_init
*函数功能：存储模块初始化函数
*输入参数：无
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int storage_unit_init()
{
	INFO_PRINT("\n----->Storage_unit 模块初始化..........\n");
	
	//计数器
	global_shared.retain_count++;
	
	//数据库操作初始化
	if(storage_unit_database_init()){
		log_write(LOG_ERROR, "storage_unit_database_init() error\n");
		return YT_FAILED;
	}
	
	//定时初始化
	if(storage_unit_timer_init()){
		log_write(LOG_ERROR, "storage_unit_timer_init() error\n");
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：storage_unit_storage_data
*函数功能：处理插件传来的数据
*输入参数：storage_data:插入数据   thread_num:线程号
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int storage_unit_storage_data(char *storage_data, const int thread_num)
{
	return storage_unit_database_insert(storage_data, thread_num);
}

/*
*函数名称：storage_unit_destroy
*函数功能：存储模块释放函数
*输入参数：无
*输出参数：无
*返回值：  无	 
*/
void storage_unit_destroy()
{
	INFO_PRINT("\n----->Storage_unit 模块释放..........\n");
	
	//计数器
	global_shared.retain_count--;
	
	storage_unit_database_destory();
	storage_unit_timer_destory();
}
