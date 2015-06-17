/***********************************************************
 *文件名：pthread_affinity.c
 *创建人：小菜_默
 *日  期：2013年10月15日
 *描  述：pthread affinity process file .c
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#include "pthread_affinity.h"
#include "stdyeetec.h"

#include <sys/types.h>

#define __USE_GNU
#include <pthread.h>

/*
 *函数名称：pthread_affinity_set
 *函数功能：set pthread affinity
 *输入参数：core_mask          set core mask
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int pthread_affinity_set(unsigned core_mask)
{
	cpu_set_t mask;
	CPU_ZERO(&mask);
	CPU_SET(core_mask,&mask);

	if(pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask)){
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
 *函数名称：pthread_affinity_get
 *函数功能：get pthread mask
 *输入参数：pthread_id         set thread pid
 *输出参数：*pthread_mask      return pthread mask
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int pthread_affinity_get(int pthread_id,unsigned *pthread_mask)
{
	return YT_SUCCESSFUL;
}

