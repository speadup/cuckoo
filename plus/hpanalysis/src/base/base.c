#include "stdyeetec.h"
#include "yt_signal.h"

/*
 *函数名称: base_init
 *函数功能: init base model
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int base_init(void)
{
	INFO_PRINT("\n----->Base模块初始化..........\n");
	global_shared.retain_count ++;
	return base_signal_set();
}

/*
 *函数名称: base_destroy
 *函数功能: destory base model
 *输入参数: none
 *输出参数: none
 *返回值  : return 0(YT_SUCCESSFUL)
*/
int base_destroy(void)
{
	INFO_PRINT("\n----->Base模块释放..........\n");
	global_shared.retain_count --;
	return YT_SUCCESSFUL;
}
