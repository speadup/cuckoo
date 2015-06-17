/***********************************************************
*文件名：plugin_debug.h
*创建人：韩涛
*日  期：2014年1月17日
*描  述：插件debug头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#ifndef _PLUGIN_DEBUG_H_
#define _PLUGIN_DEBUG_H_

/////////////////普通打印debug//////////////////////

#ifdef PLUGIN_PRINT_DEBUG

#define PLUGIN_PRINT(str) fprintf(stdout, "%s %s [%d LINE] %s\n", __FILE__, __FUNCTION__, __LINE__, str)

#else

#define PLUGIN_PRINT(str)

#endif

#ifdef PLUGIN_MEM_DEBUG

#define DEBUG_MEM_PRINT(str, args...) fprintf(stdout, "[%s] [%s] [%d LINE]" str, __FILE__, __FUNCTION__, __LINE__, ##args)

#else

#define DEBUG_MEM_PRINT(str, args...)

#endif

/////////////////debug截取结果//////////////////////
#ifdef RESULT_DEBUG

#define RESULT_PRINT(mem, result) fprintf(stdout, "-----------%s: %s---------------\n", mem, result)

#else

#define RESULT_PRINT(mem, result)

#endif


#endif
