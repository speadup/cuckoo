/***********************************************************
 *文件名：net_defs.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：net 模块对外的结构体
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _NET_DFES_H_
#define _NET_DFES_H_

#include "atomic.h"

#define SKIP_MAC   12  //跳过mac地址
#define MAC_LEN     6  //mac addr len
#define IP6_LEN    16  //ipv6 ip len

#define YT_IS_TCP  0   //is tcp
#define YT_IS_UDP  1   //is udp

//net tuple 
struct net_tuple{
	unsigned char mac_dest[MAC_LEN];     //destination eth addr
	unsigned char mac_source[MAC_LEN];   //source ether addr

	unsigned int  ip4_saddr;             //be32 type ipv4 source addr
	unsigned int  ip4_daddr;             //be32 type ipv4 dest addr

	unsigned int  ip6_saddr[4];          //be32 type ipv6 source addr
	unsigned int  ip6_daddr[4];          //be32 type ipv6 dest addr

	unsigned short port_source;          //be16 type source port 
	unsigned short port_dest;            //be16 type dest port

	unsigned char  l4_type;              //4 layer type
};

//net packet info
struct packet_info{
	struct net_tuple tuple;              //tuple数据
	unsigned short vlan_id;              //vlan id
	unsigned int   pkt_size;             //packet size;
	unsigned short client_or_server;     //表示数据方向 FROM_*_DATA
};

//flag from client data or from server data
//顺序不可移动
enum {
	FROM_CLIENT_DATA,
	FROM_SERVER_DATA,
	FROM_NONE_DATA
};

struct session_deal_record{
	void *record;						//会话处理记录
	unsigned char session_status;       //会话状态
	struct list_head *session_index;	//会话处理包位置

	char session_deal_flag;				//会话正在处理标志 0-没处理 1-正在处理
#define SESSION_CAN_DEAL 0
#define	SESSION_NOT_DEAL 1

	char record_destory_flag;			//record释放标志 0-不可以释放 1-可以释放
#define RECORD_NOT_DESTORY 0
#define RECORD_CAN_DESTORY 1

	atomic_t record_counter;            //记录中转链的未释放计数

	int (*func_plugin_deal)(struct data_transit *,int thread_num);    //处理会话的插件函数	
};

#endif  //_NET_DFES_H_

