/***********************************************************
 *文件名：udp.c
 *创建人：小菜_默
 *日  期：2014年08月12日
 *描  述：udp数据处理件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/
#include "udp.h"
#include "stdyeetec.h"
#include "net_debug.h"
#include "net_defs.h"
#include "mempool.h"
#include "data_transit.h"

#include <linux/udp.h>

/*
 *函数名称：udp_init
 *函数功能：完成udp模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int udp_init()
{
	INFO_PRINT("Net->Udp模块初始化\n");
	return YT_SUCCESSFUL;
}

/*
 *函数名称：udp_destroy
 *函数功能：释放udp模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void udp_destroy()
{
	INFO_PRINT("Net->Udp模块释放\n");
}

/*
 *函数名称：udp_input
 *函数功能：处理udp数据
 *输入参数：pkt_info           数据包信息
 *          udp                udp数据
 *          udp_size           udp数据的的大小
 *输出参数：none
 *返回值：  none
*/
void udp_input(struct packet_info *pkt_info,const char *udp,const unsigned udp_size)
{
	struct udphdr *udp_head = (struct udphdr*)udp;
	int udp_data_size = udp_size - sizeof (struct udphdr);

    pkt_info->tuple.l4_type     = YT_IS_UDP;
	pkt_info->tuple.port_source = udp_head->source;
	pkt_info->tuple.port_dest   = udp_head->dest;
/*
	struct session_deal_record *deal_record = mempool_malloc(sizeof(struct session_deal_record));
	if(!deal_record){
		log_write(LOG_ERROR,"Net->Udp->deal_record内存分配失败[%d line].\n",__LINE__);
		return ;
	}

	deal_record->session_status = SESSION_STATUS_RECV;
	deal_record->session_index  = NULL;

	data_transit_put(deal_record, pkt_info, udp, udp_size, SESSION_STATUS_RECV);
*/
//	TCP_PRINT_PORT("SPORT",ntohs(udp_head->source));
//	TCP_PRINT_PORT("DPORT",ntohs(udp_head->dest));

//	tcp_data_process(pkt_info,tcp_head,thl,tcp_data_size);
	return ;
}

