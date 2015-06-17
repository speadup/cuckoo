/***********************************************************
*文件名：sqlite_database.h
*创建人：韩涛
*日  期：2013年10月16日
*描  述：feature_manager_database模块头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _FEATURE_MANAGER_DATABASE_H_
#define _FEATURE_MANAGER_DATABASE_H_	

#include "sqlite3.h"

typedef int (sql_query_callback)(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram);

int sqlite_database_open(sqlite3 **db);

int sqlite_database_select(sqlite3 *db, char *sql, sql_query_callback *query_callback, void *callback_pram);

int sqlite_database_insert(sqlite3 *db, char *sql);

int sqlite_database_begin_transaction(sqlite3 *db);

int sqlite_database_commit_transaction(sqlite3 *db);

void sqlite_database_close(sqlite3 *db);

void sqlite_database_free(char **presult);

void sqlite_database_destroy();

#endif

