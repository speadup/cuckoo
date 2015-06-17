/***********************************************************
 *文件名：net.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：net 模块处理实现部分
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "net.h"
#include "stdyeetec.h"
#include "ethernet.h"
#include "ipv4.h"
#include "ipv6.h"
#include "tcp.h"

/*
 *函数名称：net_init
 *函数功能：完成net模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int net_init()
{
	INFO_PRINT("\n----->Net模块初始化..........\n");
	int ret = YT_SUCCESSFUL;

	//计数器
	global_shared.retain_count ++;

	ret = tcp_init ();
	if (YT_SUCCESSFUL != ret) {
		goto return_net_init;
	}

	ret = ipv4_init ();
	if (YT_SUCCESSFUL != ret) {
		goto return_net_init;
	}

return_net_init:
	return ret;
}

/*
 *函数名称：net_destroy
 *函数功能：释放net模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void net_destroy()
{
	INFO_PRINT("\n----->Net模块释放..........\n");
	tcp_destroy ();
	ipv4_destroy ();

	//计数器
	global_shared.retain_count --;
}

/*
 *函数名称：net_data_deal
 *函数功能：通过net模块处理数据的函数
 *输入参数：net_data           net 数据
 *          net_data_size      数据的大小
 *          socket_id          socket id
 *输出参数：none
 *返回值：  none
*/
void net_data_deal(const char *net_data,const int net_data_size)
{
	ethernet_input(net_data, net_data_size);
}

