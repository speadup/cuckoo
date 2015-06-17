/***********************************************************
*文件名：feature_manager_database.h
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

typedef int (sql_query_callback)(char **row_data, int row_num, int field_num, void *callback_pram);

int feature_manager_database_init();

int feature_manager_database_query(char *sql, sql_query_callback *query_callback, void *callback_pram);

void feature_manager_database_destroy();

#endif

