/***********************************************************
 *文件名：public.c
 *创建人：小菜_默
 *日  期：2013年10月25日
 *描  述：public interface process
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "public.h"
#include "stdyeetec.h"

#include "hash.h"
#include "log.h"
#include "mempool.h"

/*
 *函数名称：public_init
 *函数功能：public module init
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int public_init()
{
	INFO_PRINT("\n----->Public模块初始化..........\n");

	//计数器
	global_shared.retain_count ++;

	mempool_init();
	init_hash();
	log_init();

	return YT_SUCCESSFUL;
}

/*
 *函数名称：public_destroy
 *函数功能：public module destroy
 *输入参数：none
 *输出参数：none
 *返回值：  none 
*/
void public_destroy()
{
	INFO_PRINT("\n----->Public模块释放..........\n");

	log_destroy();
	mempool_destroy();

	//计数器
	global_shared.retain_count --;
}

