/***********************************************************
*文件名：data_transit_debug.h
*创建人：zhaixingwei
*日  期：2014年1月26日
*描  述：debug头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _DATA_TRANSIT_DEBUG_H_
#define _DATA_TRANSIT_DEBUG_H_

#ifdef DATA_TRANSIT_DEBUG

#define DATA_TRANSIT_DPRINT(fmt, args...) fprintf(stdout, "[%s][%s][%d] "fmt, __FILE__, __FUNCTION__, __LINE__, ##args)

#else

#define DATA_TRANSIT_DPRINT(fmt, args...)

#endif

#ifdef DATA_TRANSIT_MEM_DEBUG

#define DATA_MEM_PRINT(str, args...) fprintf(stdout, "[%s] [%s] [%d LINE]" str, __FILE__, __FUNCTION__, __LINE__, ##args)

#else

#define DATA_MEM_PRINT(str, args...)

#endif

#endif
