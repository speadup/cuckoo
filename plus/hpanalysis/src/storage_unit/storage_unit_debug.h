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

#ifndef _STORAGE_UNIT_DEBUG_H
#define _STORAGE_UNIT_DEBUG_H

///////////////////////路径打印/////////////////////////
#ifdef PATH_DEBUG_PPRINT

#define PATH_PRINT(mes, str) fprintf(stdout, "storage unit %s [%d line] %s:%s\n", __FILE__, __LINE__, mes, str);

#else

#define PATH_PRINT(mes, str)

#endif

/////////////////////核号、线程数打印/////////////////
#ifdef NUM_DEBUG_PRINT

#define NUM_PRINT(mes, num) fprintf(stdout, "storage unit %s [%d line] %s:%d\n", __FILE__, __LINE__, mes, num);

#else

#define NUM_PRINT(mes, num)

#endif


#endif
