/***********************************************************
 *文件名：mempool.h
 *创建人：小菜_默
 *日  期：2013年10月25日
 *描  述：mallco mempool
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _MEM_POOL_H_
#define _MEM_POOL_H_

/*
 *函数名称：mempool_init
 *函数功能：mempool init
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int mempool_init();

/*
 *函数名称：mempool_destroy
 *函数功能：destroy mempool module
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void mempool_destroy();

/*
 *函数名称：mempool_malloc
 *函数功能：mempool return malloc_size memory
 *输入参数：malloc_size        malloc size
 *输出参数：none
 *返回值：  return malloc size space.
*/
void *mempool_malloc(const int malloc_size);

/*
 *函数名称：mempool_realloc
 *函数功能：realloc size memory
 *输入参数：mem_data           malloc memory allocation
 *          realloc_size       realloc size
 *输出参数：none
 *返回值：  return realloc size space
*/
void *mempool_realloc(void *mem_address,const int realloc_size);

/*
 *函数名称：mempool_free
 *函数功能：free malloc or realloc allocation memory
 *输入参数：mem_free           free memory pointer
 *输出参数：none
 *返回值：  none
*/
void mempool_free(void *mem_free);

/*
 *函数名称：mempool_display_info
 *函数功能：print mempool info
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void mempool_display_info(int signo);

/*
 *函数名称：mempool_memory_info
 *函数功能：test malloc or realloc allocation memory
 *输入参数：memory           test memory pointer
 *输出参数：none
 *返回值：  none
*/
void mempool_memory_info(void *memory,char *mem_name);

#endif  //_MEM_POOL_H_

