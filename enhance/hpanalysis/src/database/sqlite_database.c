/***********************************************************
*文件名：sqlite_database.c
*创建人：韩涛
*日  期：2013年10月16日
*描  述：sqlite_database模块相关数据库初始化，查询功能。
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*************************************************************/
#include "config.h"
#include "list.h"
#include "stdyeetec.h"
#include "sqlite_database_debug.h"
#include "sqlite_database.h"


/*
*函数名称：sqlite_database_open
*函数功能：打开sqlite数据库
*输入参数：无
*输出参数：db
*返回值：  0 成功 -1 失败
*/
int sqlite_database_open(sqlite3 **db)
{
	struct yt_config *yt_config = NULL;
	int nresult = 0;


	//调用base模块的获取参数函数，获取数据库初始化参数
	yt_config = base_get_configuration();
	
//	DB_INFO_PRINT(yt_config);
	
	nresult = sqlite3_open(yt_config->fm_db_name, db);	
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "sqlite3 open error\n");
		return nresult;
	}

	return nresult;
}	

/*
*函数名称：sqlite_database_select
*函数功能：数据库查询并回调处理函数
*输入参数：db:数据库 sql:sql查询语句 query_callback：回调处理函数 callback_pram：回调函数用到的参数
*输出参数：无
*返回值：  0 成功 -1失败
*/
int sqlite_database_select(sqlite3 *db, char *sql, sql_query_callback *query_callback, void *callback_pram)
{
	char *errmsg = NULL;
	int nresult = 0;
	int nrow = 0;
	int ncolumn = 0;
	char **presult = NULL;
	sqlite3 *db_tmp = (sqlite3*)db;

	if(!db || !sql){
		return YT_FAILED;
	}
	
	nresult = sqlite3_get_table(db_tmp, sql, &presult, &nrow, &ncolumn, &errmsg);
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "sqlite3_get_table error\n");
		DEBUG_PRINT("%s\n", errmsg);
		
		return YT_FAILED;
	}

	query_callback(db_tmp, presult, nrow, ncolumn, callback_pram);

	return YT_SUCCESSFUL;
}

/*
*函数名称：sqlite_database_insert
*函数功能：数据库插入函数
*输入参数：db:数据库  sql:sql查询语句 
*输出参数：无
*返回值：  0 成功 -1失败
*/
int sqlite_database_insert(sqlite3 *db, char *sql)
{
	int nresult = 0;
	char *errmsg = NULL;

	if(!db || !sql){
		return YT_FAILED;
	}

	nresult = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "sqlite_database_insert error\n");
		DEBUG_PRINT("%s\n", errmsg);
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;

}

/*
*函数名称：sqlite_database_begin_transaction
*函数功能：数据库开启事务函数
*输入参数：db:数据库 
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int sqlite_database_begin_transaction(sqlite3 *db)
{
	if(!db){
		return YT_FAILED;
	}
	
	int nresult = sqlite3_exec(db, "begin", 0, 0, 0);

	return nresult;
}

/*
*函数名称：sqlite_database_commit_transaction
*函数功能：数据库提交事务函数
*输入参数：db:数据库 
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int sqlite_database_commit_transaction(sqlite3 *db)
{
	if(!db){
		return YT_FAILED;
	}
	
	int nresult = sqlite3_exec(db, "commit", 0, 0, 0);

	return nresult;
}

/*
*函数名称：sqlite_database_close
*函数功能：数据库关闭函数
*输入参数：db:数据库 
*输出参数：无
*返回值：  无
*/
void sqlite_database_close(sqlite3 *db)
{
	if(!db){
		return;
	}
	
	sqlite3_close(db);
}


/*
*函数名称：sqlite_database_free
*函数功能：数据库结果集释放函数
*输入参数：presult:结果集 
*输出参数：无
*返回值：  无
*/
void sqlite_database_free(char **presult)
{
	if(!presult){
		return ;
	}
	
	sqlite3_free_table(presult);
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
	INFO_PRINT("Feature_manager->database模块释放\n");
	
}

