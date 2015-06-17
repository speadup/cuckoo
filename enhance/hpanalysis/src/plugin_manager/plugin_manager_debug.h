/***********************************************************
*文件名：plugin_manager_debug.h
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

#ifndef _PLUGIN_MANAGER_DEBUG_H_
#define _PLUGIN_MANAGER_DEBUG_H_

#ifdef PLUGIN_MANAGER_DEBUG
#define PLUGIN_MANAGER_DPRINT(fmt, args...) fprintf(stdout, "[%s][%s][%d] "fmt, __FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define PLUGIN_MANAGER_DPRINT(fmt, args...)
#endif
#endif
