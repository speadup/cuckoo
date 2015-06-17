/***********************************************************
*文件名：feature_manager_deal.h
*创建人：韩涛
*日  期：2013年10月17日
*描  述：特征码处理模块头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _FEATURE_MANAGER_DEAL_H_
#define _FEATURE_MANAGER_DEAL_H_

#include <sqlite3.h>

int feature_manager_deal_init(); 

int feature_manager_deal_main(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram);

int feature_manager_deal_mark(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram);

int feature_manager_deal_extract(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram);

struct plugin_feature *feature_manager_deal_get(int *row_num);

void feature_manager_deal_destroy();
#endif
