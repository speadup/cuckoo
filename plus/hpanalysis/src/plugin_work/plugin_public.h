/***********************************************************
*文件名：plugin_public.h
*创建人：韩涛
*日  期：2013年10月21日
*描  述：公共头文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _PLUGIN_PUBLIC_H_
#define _PLUGIN_PUBLIC_H_

#include <arpa/inet.h>

#include "base.h"
#include "list.h"
#include "config.h"
#include "mempool.h"
#include "atomic.h"
#include "tcp_session.h"
#include "stdyeetec.h"
#include "storage_unit.h"
#include "plugin_debug.h"
#include "data_transit.h"
#include "plugin_encode.h"
#include "plugin_manager.h"
#include "feature_manager.h"
#include "plugin_config_feature.h"
#include "plugin_config_macro.h"
#include <unistd.h>
#include <sys/stat.h>
#include "rand_str.h"

//元组结构体
struct plugin_element{
	char smac[MAC_LENGTH + 1];							//源MAC
	char dmac[MAC_LENGTH + 1];							//目标MAC
	char sip[IP_LENGTH + 1];							//源IPv4地址
	char dip[IP_LENGTH + 1];							//目标IPv6地址
	char sip6[IP_LENGTH + 1];							//源ipv6地址
	char dip6[IP_LENGTH + 1];							//目的ipv6地址
	unsigned short sport;								//源端口
	unsigned short dport;								//目标端口
	unsigned short vlanid;								//vlan的id编号
};

//会话类型及状态结构
struct plugin_deal_status{
	int plugin_type;
	int deal_status;
};

#define IS_CASE_PROTOCOL(buff, str) (strcasestr(buff, str))
#define IS_PROTOCOL(buff, str) (strstr(buff, str))

#define MIN(a, b) a > b? b : a 


////////////////////////////////////////////////////////////////////////
//交换源、目的IP/MAC
#define SWAP_IP_MAC(source, dest, tmp) \
do{							  \
		strcpy(tmp, source);  \
		strcpy(source, dest); \
		strcpy(dest, tmp);    \
}while(0)

#define IPSTACK_GET_MAC(pmac, paddr)								\
do{												\
	sprintf((char *)(pmac),"%02X%02X%02X%02X%02X%02X",					\
		((unsigned char *)paddr)[0],							\
		((unsigned char *)paddr)[1],							\
		((unsigned char *)paddr)[2],							\
		((unsigned char *)paddr)[3],							\
		((unsigned char *)paddr)[4],							\
		((unsigned char *)paddr)[5]							\
		);										\
}while(0)

#ifdef DEVICE_BIG_ENDIAN

#define IPSTACK_GET_IP(pip, paddr)								\
do{												\
	sprintf((char *)(pip),"%03d.%03d.%03d.%03d",						\
		((unsigned char *)(paddr))[3],							\
		((unsigned char *)(paddr))[2],							\
		((unsigned char *)(paddr))[1],							\
		((unsigned char *)(paddr))[0]							\
		);										\
}while(0)

#else

#define IPSTACK_GET_IP(pip, paddr)								\
do{												\
	sprintf((char *)(pip),"%03d.%03d.%03d.%03d",						\
		((unsigned char *)(paddr))[0],							\
		((unsigned char *)(paddr))[1],							\
		((unsigned char *)(paddr))[2],							\
		((unsigned char *)(paddr))[3]							\
		);										\
}while(0)


#endif


#define IPSTACK_GET_PORT(pport, s_port)								\
do{												\
	pport = ntohs(((unsigned short)s_port));				\
}while(0)

#define IPSTACK_GET_VLANID(vlanid, s_vlanid)							\
do{												\
	vlanid = s_vlanid;									\
}while(0)


////////////////////////////////////////////////////////////////////////
#define IPSTACK_GET_ALL_IP_MAC_PORT(pst, ppif)						\
do{												\
	IPSTACK_GET_MAC((pst)->smac, ((ppif)->mac_source));			\
	IPSTACK_GET_MAC((pst)->dmac, ((ppif)->mac_dest));			\
	IPSTACK_GET_IP((pst)->sip, (&((ppif)->ip4_saddr)));			\
	IPSTACK_GET_IP((pst)->dip, (&((ppif)->ip4_daddr)));			\
	IPSTACK_GET_PORT(((pst)->sport), ((ppif)->port_source));		\
	IPSTACK_GET_PORT(((pst)->dport), ((ppif)->port_dest));		\
}while(0)

#define GET_REVERSE_NETINFO_STRING(pst, ppif)                                                \
do{                                                                                             \
	IPSTACK_GET_MAC((pst)->smac, ((ppif)->t7.src_mac));			\
	IPSTACK_GET_MAC((pst)->dmac, ((ppif)->t7.dst_mac));			\
	IPSTACK_GET_IP((pst)->sip, (&((ppif)->t7.src_ip)));			\
	IPSTACK_GET_IP((pst)->dip, (&((ppif)->t7.dst_ip)));			\
	IPSTACK_GET_PORT(((pst)->sport), ntohs(((ppif)->t7.dst_port)));		\
	IPSTACK_GET_PORT(((pst)->dport), notes(((ppif)->t7.src_port)));		\
}while(0)

#define ZYSTRNCPY(dst, src, length)					\
do{									\
	if( (dst) && (src) && (length>0) ){				\
		strncpy((char *)(dst), (char *)(src), (length));	\
	}								\
}while(0)

////////////////////////////////////////////////////////////////////////
struct plugin_feature;
struct plugin_feature_extract;
struct list_head;
struct data_transit;

struct plugin_feature *plugin_feature_mark(char *deal_data, int data_length, struct plugin_feature *plugin_feature_head);
int plugin_feature_extract(char *deal_data, int data_len, struct list_head *plugin_fe_list_node, void *plugin_dst);
int plugin_extract_type_deal(char *deal_data, char *dst, int dst_len, struct plugin_feature_extract *bbs_fe);
int plugin_data_extract(char *deal_data, char *extract_head, char *extract_tail, char *dst, int dst_length);
int plugin_address_deal(char *deal_data, char *dst, int dst_len);
int plugin_write_file(char *put_type, char *data, int data_length, FILE *dst_file);
void convert_encodes(char *dst, int dst_len, char *encode, char *extra);
char *memstr(char *haystack, char *needle, int size);

#endif

