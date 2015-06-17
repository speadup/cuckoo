/***********************************************************
 *文件名：stdyeetec.h
 *创建人：小菜_默
 *日  期：2013年10月15日
 *描  述：public define and include 
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _STDYEETEC_H_
#define _STDYEETEC_H_

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>

#include "log.h"

#define YT_SUCCESSFUL 0    //function and other execute success returned
#define YT_FAILED    -1    //function and other execute failed  returned

#define YT_IS_TRUE    1    //return judge is true 
#define YT_IS_FALSE   0    //return judge is false

#define YT_MAX_LCORE  8    //max lcore
#define YT_MAX_DEV    4    //max dev

#define MAX_FILE_NAME_LENGTH 255    //max file name len
#define MAX_FILE_PATH_LENGTH 4096   //max file path len

//include/linux/compiler.h
#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

//debug print
#define DEBUG_PRINT(fmt, args...) fprintf(stdout, "[%s][%s][%d] "fmt, \
		__FILE__, __FUNCTION__, __LINE__, ##args)

//info print
#define INFO_PRINT(fmt, args...) fprintf(stdout, fmt, ##args)

//record program run or exit status
struct shared{
#define CONTINUE_RUN  1
#define CONTINUE_EXIT 0
	char continue_run:1, //flag 1 continue run,flag 0 is exit 
	     retain_count:4, //保留计数器，每个模块初始化时+1，退出时-1,最大支持15个模块 
	     reserve:3;      //reserve
};

extern volatile struct shared global_shared;    //global share data

#endif   //_STDYEETEC_H_

