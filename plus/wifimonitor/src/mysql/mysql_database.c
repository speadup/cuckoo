/***********************************************************
*文件名：mysql_database.c
*创建人：丁海涛
*日  期：2014年6月19日
*描  述：mysql_database模块相关数据库初始化，查询功能。
*
*修改人：
*日  期：
*描  述：
*************************************************************/
#include "mysql_database.h"

static MYSQL *mysql_conn = NULL;

/*
*函数名称：mysql_database_init
*函数功能：打开mysql数据库
*输入参数：无
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int mysql_database_init()
{
	mysql_conn = mysql_init(NULL);

	if(!mysql_real_connect(mysql_conn, MYSQL_SERVER, MYSQL_USER, MYSQL_PASS, MYSQL_DATABSE, 0, NULL, 0)){
		//log_write(LOG_ERROR, "mysql connect error\n");
		return -1;
	}
	//关闭自动事务
	//mysql_query("SET AUTOCOMMIT=0");
	return 0;
}

/*
*函数名称：mysql_database_query_row
*函数功能：从数据库查询结果行中找到需要的数据
*输入参数：sql:sql查询语句 query_callback：回调处理函数 callback_pram：回调函数用到的参数
*输出参数：无
*返回值：  0 成功 -1失败
 */
int mysql_database_query_row(char *sql, query_row_callback *query_callback, void *callback_pram)
{
	MYSQL_RES *res;
	MYSQL_ROW row_data;

	if(mysql_database_exec_sql(sql) !=0)
		return -1;


	res = mysql_store_result(mysql_conn);

	if(!res)
		return -1;

    do{
	    row_data = mysql_fetch_row(res);
		if(query_callback)
			if(query_callback(row_data, callback_pram) < 0)
                break;
	}while(row_data != NULL);

	mysql_free_result(res);

	return 0;
}

/*
*函数名称：mysql_database_query_dataset
*函数功能：从数据库查询结果集中找到需要的数据
*输入参数：sql:sql查询语句 query_callback：回调处理函数 callback_pram：回调函数用到的参数
*输出参数：无
*返回值：  0 成功 -1失败
 */
int mysql_database_query_dataset(char *sql, query_dataset_callback *query_callback, void *callback_pram)
{
	MYSQL_RES *res;
	int row_num;

	if(mysql_database_exec_sql(sql) !=0)
		return -1;

	res = mysql_store_result(mysql_conn);

	if(!res)
		return -1;

	row_num = mysql_num_rows(res);

	if(query_callback)
		query_callback(res, row_num, callback_pram);

	mysql_free_result(res);

	return 0;
}

/*
*函数名称：mysql_database_query_seek
*函数功能：设置数据集中行偏移
*输入参数：res:查询数据集
*输出参数：无
*返回值：  成功返回指定行数据，否则返回NULL
 */
MYSQL_ROW mysql_database_seek(MYSQL_RES *res, int row)
{
	MYSQL_ROW row_data;

	mysql_data_seek(res, row);

	if((row_data = mysql_fetch_row(res)) != NULL)
		return row_data;
	
	return NULL;
}

/*
*函数名称：mysql_database_exec_sql
*函数功能：执行sql语句
*输入参数：sql:sql查询语句 
*输出参数：无
*返回值：  0 成功 -1失败
*/
int mysql_database_exec_sql(char *sql)
{
	if(!sql || !mysql_conn)
		return -1;

	if(mysql_query(mysql_conn, sql)){
		mysql_ping(mysql_conn);
		return mysql_query(mysql_conn, sql);
	}

	return 0;
}

/*
*函数名称：mysql_database_close
*函数功能：数据库关闭函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void mysql_database_close()
{
	mysql_close(mysql_conn);
}
