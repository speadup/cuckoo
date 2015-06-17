/***********************************************************
*文件名：feature_manager_place.h
*创建人：韩涛
*日  期：2013年12月2日
*描  述：place信息获取头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _FEATURE_MANAGER_PLACE_H_
#define _FEATURE_MANAGER_PLACE_H_
#include <sqlite3.h>

int feature_manager_place_init();

int feature_manager_place_deal(sqlite3 *db, char **row_data, int row_num, int ncolumn, void *callback_pram);

void feature_manager_place_destory();

#endif
