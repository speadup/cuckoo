/***********************************************************
 *文件名：ipv4.c
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：解析ipv4数据的实现部分
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#include "ipv4.h"
#include "stdyeetec.h"

#include "net_defs.h"
#include "tcp.h"
#include "udp.h"
#include "ip_fragment.h"
#include "mempool.h"
#include "net_debug.h"

//大小端转换头函数使用的文件
#include <arpa/inet.h>  //该头文件包含了netinet/in.h 
                        //in.h包含了三层协议的定义IPPROTO_*
#include <linux/ip.h>

#define IP_HEAD_LEN(ihl) ((ihl) << 2)

void ipv4_switch(struct packet_info *pkt_info, const char *net_data, const unsigned net_data_size,
				 const unsigned short proto);

/*
 *函数名称：ipv4_init
 *函数功能：ipv4处理模块初始化
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int ipv4_init ()
{
	return YT_SUCCESSFUL;
}

/*
 *函数名称：ipv4_destroy
 *函数功能：ipv4处理模块资源释放
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void ipv4_destroy ()
{
}

/*
 *函数名称：ipv4_input
 *函数功能：处理ipv4数据的入口函数
 *输入参数：pkt_info           数据包信息
 *          ipv4_data          ipv4数据
 *          ipv4_data_size     数据大小
 *输出参数：none
 *返回值：  none
*/
void ipv4_input(struct packet_info *pkt_info, const char *ipv4_data,
		        unsigned ipv4_data_size)
{
	struct iphdr *ip_head = (struct iphdr*)ipv4_data;
	int ihl = IP_HEAD_LEN(ip_head->ihl);

	IPv4_PRINT_IP("SIP",&(ip_head->saddr));
	IPv4_PRINT_IP("DIP",&(ip_head->daddr));

	//判断头部大小是否正确
	if(UNLIKELY(ihl < sizeof(struct iphdr))){
		mempool_free(pkt_info);
		return ;
	}

    //判断数据长度是否正确 去定二层的padding
    if(UNLIKELY(ntohs(ip_head->tot_len) != ipv4_data_size)) {
		ipv4_data_size = ntohs(ip_head->tot_len);
		//mempool_free(pkt_info);
		//return ;
    }

	pkt_info->tuple.ip4_saddr = ip_head->saddr;
	pkt_info->tuple.ip4_daddr = ip_head->daddr;

	//此处判断是否是碎片包
	//if(ip_fragment_input()){
	//	return ;
	//}
	
	ipv4_switch(pkt_info,ipv4_data+ihl,ipv4_data_size-ihl,
				ip_head->protocol);
}

/*
 *函数名称：ipv4_switch
 *函数功能：四层协议选择
 *输入参数：pkt_info          数据包信息
 *          net_data          net数据
 *          net_data_size     数据大小
 *          proto             协议类型
 *输出参数：none
 *返回值：  none
*/
void ipv4_switch(struct packet_info *pkt_info, const char *net_data, const unsigned net_data_size,
				 const unsigned short proto)
{
	switch(proto){
		case IPPROTO_TCP: 
			tcp_input(pkt_info,net_data,net_data_size);
			break;

		case IPPROTO_UDP: 
//			udp_input(pkt_info,net_data,net_data_size);
//			break;

		case IPPROTO_IPV6:
		case IPPROTO_ICMP:
		case IPPROTO_GRE:
		default:
			mempool_free (pkt_info);
	}
}

