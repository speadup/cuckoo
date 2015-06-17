/***********************************************************
 *文件名：ethernet.c
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：二层数据处理的
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "ethernet.h"
#include "stdyeetec.h"

#include "ipv4.h"
#include "ipv6.h"
#include "mempool.h"
#include "net_defs.h"
#include "net_debug.h"

#include <linux/if_ether.h>
#include <arpa/inet.h>

#define VLAN_VID_MASK   0x0fff      //VLAN Identifier
#define VLAN_HLEN       4

/*
 *  struct vlan_hdr - vlan header
 *  @h_vlan_TCI: priority and VLAN ID
 *  @h_vlan_proto: packet type ID or len
 */
//linux-2.6.35.13/include/linux/if_vlan.h
struct vlan_hdr {
    __be16  h_vlan_TCI;
    __be16  h_vlan_proto;
};
/*
 vhdr = (struct vlan_hdr *)skb->data;
 vlan_tci = ntohs(vhdr->h_vlan_TCI);
 vlan_id = vlan_tci & VLAN_VID_MASK;
*/

static void ethernet_vlan(struct packet_info *pkt_info,const char *vlan_data,
		                  const unsigned vlan_data_size);

static void ethernet_switch(struct packet_info *pkt_info,const char *net_data,
		                    const unsigned net_data_size,const unsigned short proto);

/*
 *函数名称：ethernet_input
 *函数功能：接收二层的数据包进行处理
 *输入参数：eth_data           二层数据
 *          eth_data_size      数据大小
 *输出参数：none
 *返回值：  none
*/
void ethernet_input(const char *eth_data,const unsigned data_size)
{
	//size 减去帧头和CRC小于等于0 说明是一个非法帧
	if(UNLIKELY(data_size-sizeof(struct ethhdr) <= 0)){
		return ;
	}

	//在后面的判断中，不要忘记该内存的释放
	struct packet_info *pkt_info = (struct packet_info*)mempool_malloc(sizeof(struct packet_info));
	if (!pkt_info) {
		log_write(LOG_ERROR,"[%s][%s][%d] 内存分配失败\n",__FILE__,__func__,__LINE__);
		return;
	}

	//给packet_info结构体赋值
	struct ethhdr *eth_addr = (struct ethhdr*)eth_data;
	pkt_info->vlan_id       = 0;
	pkt_info->pkt_size      = data_size;
	memcpy(pkt_info->tuple.mac_dest  ,eth_addr->h_dest  ,ETH_ALEN);
	memcpy(pkt_info->tuple.mac_source,eth_addr->h_source,ETH_ALEN);

	ETH_PRINT_MAC("SMAC",eth_addr->h_source);
	ETH_PRINT_MAC("DMAC",eth_addr->h_dest);

	ethernet_switch(pkt_info,eth_data+ETH_HLEN,data_size-ETH_HLEN,ntohs(eth_addr->h_proto));
}

/*
 *函数名称：ethernet_vlan
 *函数功能：解析vlan数据获取vlan_id
 *输入参数：pkt_info           数据包的信息
 *          vlan_data          vlan 数据
 *          vlan_data_size     数据大小
 *输出参数：none
 *返回值：  none
*/
static void ethernet_vlan(struct packet_info *pkt_info,const char *vlan_data,
		                  const unsigned vlan_data_size)
{
	struct vlan_hdr *vlan_head = (struct vlan_hdr*)vlan_data;
	short  vlan_tci = ntohs(vlan_head->h_vlan_TCI);
	pkt_info->vlan_id = vlan_tci & VLAN_VID_MASK;

	ethernet_switch(pkt_info,vlan_data+VLAN_HLEN,
			        vlan_data_size-VLAN_HLEN,
			        ntohs(vlan_head->h_vlan_proto));
}

/*
 *函数名称：ethernet_switch
 *函数功能：协议类型选择
 *输入参数：pkt_info           数据包的信息
 *          net_data           net 数据
 *          net_data_size      数据大小
 *          proto              协议类型
 *输出参数：none
 *返回值：  none
*/
static void ethernet_switch(struct packet_info *pkt_info,const char *net_data,
		                    const unsigned net_data_size,const unsigned short proto)
{
	switch(proto){
		case ETH_P_IP:   
			 ipv4_input(pkt_info,net_data,net_data_size);
			 break;

		case ETH_P_IPV6: 
			 //ipv6_input(pkt_info,net_data,net_data_size);
			 //break;

		case ETH_P_8021Q:
			 ethernet_vlan(pkt_info,net_data,net_data_size);
			 break;

		case ETH_P_ARP: 
		case ETH_P_RARP:
		default:
			 mempool_free (pkt_info);
	}
}


