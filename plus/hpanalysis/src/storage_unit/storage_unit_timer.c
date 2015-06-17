/***********************************************************
*文件名：storage_unit_timer.c
*创建人：韩涛
*日  期：2013年11月5日
*描  述：存储模块定时模块源文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#include "stdyeetec.h"
#include "config.h"
#include "storage_unit_debug.h"
#include "storage_unit_database.h"
#include <unistd.h>
#include "pthread_affinity.h"
#include <time.h>
#include <pthread.h>

static pthread_t gthread_id = 0; //超时检测线程的id号
void *storage_check_time_thread(void *arg);

time_t time_head = 0;

/*
*函数名称：storage_unit_timer_init
*函数功能：定时器初始化
*输入参数：
*输出参数：
*返回值：
*/
int storage_unit_timer_init()
{
	INFO_PRINT("Storage_unit_time模块初始化\n");

	time_head = time(0);

	if(YT_SUCCESSFUL != pthread_create(&gthread_id, NULL, storage_check_time_thread, NULL)){
		log_write(LOG_ERROR, "storage_unit_timer_init pthread_create error\n");
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}


/*
*函数名称：storage_check_time_thread
*函数功能：超时线程
*输入参数：无
*输出参数：无
*返回值：  0 成功 -1 失败
*/
void *storage_check_time_thread (void *arg)
{
	struct yt_config *yt_config_tmp = NULL;

	yt_config_tmp = base_get_configuration();
	
	if(!yt_config_tmp){
		log_write(LOG_ERROR, "storage_unit_timer_init() base_get_configuration() error\n");
		return NULL;
	}

	NUM_PRINT("数据入库(提交事务)的时间间隔", yt_config_tmp->storage_w2f_timeout);

	static int i = 0;
	//查找使用的哪个cpu
	for (i=0;i<YT_MAX_LCORE;i++) {
		//判断并设定线程到指定cpu
		if (yt_config_tmp->extpthread_on_lcore>>i & 1) {
			NUM_PRINT("绑定核号", i);
			pthread_affinity_set(i);
			
			break;
		}
	}
	
	//永不退出的循环检测
	while (global_shared.continue_run){
		if(time(0) - time_head > yt_config_tmp->storage_w2f_timeout){
			storage_unit_database_overtime_deal();
			time_head = time(0);
		}
		
		//循环等待是为了程序退出时快
		for (i = 0; i < yt_config_tmp->storage_w2f_timeout && global_shared.continue_run; i++) {
			sleep(1);
		}
	}
	
	return NULL;
}

/*
*函数名称：storage_unit_timer_destory
*函数功能：释放函数
*输入参数：
*输出参数：
*返回值：
*/
void storage_unit_timer_destory()
{
	INFO_PRINT("Storage_unit_time模块释放\n");
	pthread_join(gthread_id,NULL);
}
