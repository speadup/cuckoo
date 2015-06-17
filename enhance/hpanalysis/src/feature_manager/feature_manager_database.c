/***********************************************************
*文件名：feature_manager_database.c
*创建人：韩涛
*日  期：2013年10月16日
*描  述：feature_manager_database模块相关数据库初始化，查询功能。
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*************************************************************/
#include "config.h"
#include "stdyeetec.h"
#include "feature_manager_debug.h"
#include "feature_manager_database.h"
#include <sqlite3.h>

#define MYSQL_TIME_OUT 30

static char **presult = NULL;

sqlite3 *feature_db;

/*
*函数名称：feature_manager_database_init
*函数功能：feature_manager_database_init模块初始化函数,初始化数据库
*输入参数：无
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int feature_manager_database_init()
{
	INFO_PRINT("FEATURE_MANAGER->DATABASE模块初始化\n");
	struct yt_config *yt_config = NULL;
	int nresult = 0;

	//调用base模块的获取参数函数，获取数据库初始化参数
	yt_config = base_get_configuration();
	
//	DB_INFO_PRINT(yt_config);
	
	nresult = sqlite3_open(yt_config->fm_db_name, &feature_db);	
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "sqlite3 open error\n");
		
		return YT_FAILED;
	}


	return YT_SUCCESSFUL;
}	

/*
*函数名称：feature_manager_database_query
*函数功能：数据库查询并回调处理函数
*输入参数：sql:sql查询语句 query_callback：回调处理函数 callback_pram：回调函数用到的参数
*输出参数：无
*返回值：  0 成功 -1失败
*/
int feature_manager_database_query(char *sql, sql_query_callback *query_callback, void *callback_pram)
{
	char *errmsg = NULL;
	int nresult = 0;
	int nrow = 0;
	int ncolumn = 0;

	nresult = sqlite3_get_table(feature_db, sql, &presult, &nrow, &ncolumn, &errmsg);
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "sqlite3_get_table error\n");
		DEBUG_PRINT("%s\n", errmsg);
		
		return YT_FAILED;
	}

	query_callback(presult, nrow, ncolumn, callback_pram);



	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_manager_database_destroy
*函数功能：feature_manager_database模块释放函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void feature_manager_database_destroy()
{
	INFO_PRINT("FEATURE_MANAGER->DATABASE模块释放\n");
	
	sqlite3_free_table(presult);
	sqlite3_close(feature_db);
}

