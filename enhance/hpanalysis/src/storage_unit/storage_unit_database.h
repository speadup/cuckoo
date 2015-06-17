
/***********************************************************
*文件名：storage_unit_database.h
*创建人：韩涛
*日  期：2013年11月5日
*描  述：存储模块数据库操作头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/


#ifndef STORAGE_UNIT_DATABASE_H_
#define STORAGE_UNIT_DATABASE_H_

#include <sqlite3.h>

int storage_unit_database_init();

int storage_unit_database_insert(char *sql, const int thread_num);

int storage_unit_database_overtime_deal();

void storage_unit_database_destory();

#endif
