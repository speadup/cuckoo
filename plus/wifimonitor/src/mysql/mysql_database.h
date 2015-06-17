/***********************************************************
*文件名：mysql_database.h
*创建人：丁海涛
*日  期：2014年6月19日
*描  述：feature_manager_database模块头文件
*
*修改人：
*日  期：
*描  述：
*
************************************************************
*/

#ifndef _FEATURE_MANAGER_DATABASE_H_
#define _FEATURE_MANAGER_DATABASE_H_	

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <mysql.h>

#define MYSQL_SERVER 	"localhost"
#define MYSQL_USER	 	"root"
#define MYSQL_PASS	 	"2allyeetecsrone"
#define MYSQL_DATABSE	"cliff"

typedef int (query_row_callback)(MYSQL_ROW row_data, void *callback_pram);
typedef int (query_dataset_callback)(MYSQL_RES *res_data, int row_num, void *callback_pram);

int mysql_database_init();

int mysql_database_query_row(char *sql, query_row_callback *query_callback, void *callback_pram);

int mysql_database_query_dataset(char *sql, query_dataset_callback *query_callback, void *callback_pram);

MYSQL_ROW mysql_database_seek(MYSQL_RES *res, int row);

int mysql_database_exec_sql(char *sql);

void mysql_database_close();

#endif
