/***********************************************************
 *文件名：ipv6.c
 *创建人：小菜_默
 *日  期：2013年11月26日
 *描  述：处理ipv6数据，暂时不处理
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "ipv6.h"
#include "stdyeetec.h"
#include "net_defs.h"

/*
 *函数名称：ipv6_input
 *函数功能：处理ipv6数据的入口函数
 *输入参数：pkt_info           包数据信息
 *          ipv6_data          ipv6数据
 *          ipv6_data_size     数据大小
 *          socket_id          socket id
 *输出参数：none
 *返回值：  none
*/
void ipv6_input(struct packet_info *pkt_info,const char *ipv6_data,
		        const unsigned ipv6_data_size,const char socket_id)
{
}

