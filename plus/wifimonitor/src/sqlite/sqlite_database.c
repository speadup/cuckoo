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
#include <stdio.h>
#include "sqlite_database.h"

/*
*函数名称：sqlite_database_open
*函数功能：打开sqlite数据库
*输入参数：无
*输出参数：db
*返回值：  0 成功 -1 失败
*/
char *g_database = NULL;

int sqlite_database_open(sqlite3 **db)
{
	int nresult = 0;

    if(!g_database)
        return -1;

	nresult = sqlite3_open(g_database, db);	
	if(nresult != SQLITE_OK){
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
		return -1;
	}
	
	nresult = sqlite3_get_table(db_tmp, sql, &presult, &nrow, &ncolumn, &errmsg);
	if(nresult != SQLITE_OK){
		return -1;
	}

	query_callback(db_tmp, presult, nrow, ncolumn, callback_pram);

	return 0;
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
		return -1;
	}

	nresult = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
	if(nresult != SQLITE_OK){
		return -1;
	}

	return 0;

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
		return -1;
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
		return -1;
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
