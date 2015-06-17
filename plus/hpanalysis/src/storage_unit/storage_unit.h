/***********************************************************
*文件名：storage_unit.h
*创建人：韩涛
*日  期：2013年11月5日
*描  述：存储模块头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _STORAGE_UNIT_H_
#define _STORAGE_UNIT_H_


int storage_unit_init();
int storage_unit_storage_data(char *storage_data, const int thread_num);
void storage_unit_destroy();

#endif
