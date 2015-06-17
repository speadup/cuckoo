/***********************************************************
 *文件名：tcp_session.h
 *创建人：小菜_默
 *日  期：2013年11月06日
 *描  述：tcp组包模块的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _TCP_SESSION_H_
#define _TCP_SESSION_H_

#include <stdint.h>
#include <time.h>
#include "list.h"
#include "net_defs.h"
#include "atomic.h"

//获取tcp头部+opt部分的长度
#define TCP_HEAD_LEN(doff) ((doff) << 2)

struct data_transit;
struct session_deal_record;

//存储早到的tcp数据包
struct tcp_early_packet{
	struct list_head info_node;         //双向链表节点
	struct tcphdr *tcp_head;            //tcp数据
	int    thl;                         //tcp头部长度
	unsigned tcp_data_size;             //去掉tcp头的数据大小
};

//每个tcp链接存储的结构体，使用拉链法解决hash冲突
struct tcp_session{
	struct list_head zip_node;          //tcp_session的节点
	struct list_head tcp_early_head;    //保存早先时候到的tcp数据

	struct packet_info *pkt_info;		//会话信息
	struct net_tuple   tuple;           //会话元组
	char   *data_buffer;                //组包后的tcp
	unsigned buf_size;                  //tcp_buffer分配的空间也是数据大小
	unsigned pkt_size;                  //packet size

	unsigned seq[2];                    //客户端与服务器的seq
	unsigned ack_seq[2];                //客户端与服务器的ack
	char     status[2];                 //客户端与服务器的会话状态

	unsigned hash_index;                //hash value
	time_t time;                      //记录链接的时间
	
	struct session_deal_record *deal_record;
};

/*
 *函数名称：tcp_session_init
 *函数功能：对组包模块进行初始化
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_init();

/*
 *函数名称：tcp_session_destroy
 *函数功能：停止模块工作释放该模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_session_destroy();

/*
 *函数名称：tcp_session_find
 *函数功能：查找tcp链接是否存在
 *输入参数：pktinfo            pkt包的信息
 *          tcp_head           tcp数据
 *输出参数：hash               返回hash值
 *返回值：  找到数据返回，查找失败返回NULL
*/
struct tcp_session *tcp_session_find(struct packet_info *pkt_info, struct tcphdr *tcp_data,
									 int thl,int tcp_data_size,unsigned *hash);

/*
 *函数名称：tcp_session_first
 *函数功能：处理tcp的第一次握手
 *输入参数：pkt_info           包信息
 *          tcp_head           tcp数据
 *          hash               该包对应的hash value
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_first(struct packet_info *pkt_info,struct tcphdr *tcp_head,
		              unsigned hash);

/*
 *函数名称：tcp_session_second
 *函数功能：tcp的第二次握手
 *输入参数：session            tcp session
 *          tcp_head           tcp数据
 *输出参数：none
 *返回值：  none
*/
void tcp_session_second(struct tcp_session *session,struct tcphdr *tcp_head);

/*
 *函数名称：tcp_session_third
 *函数功能：处理tcp的第三次握手
 *输入参数：session            tcp session
 *          tcp_head           tcp数据
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_third(struct tcp_session *session,struct tcphdr *tcp_head);

/*
 *函数名称：tcp_session_rst
 *函数功能：处理rst数据包
 *输入参数：session            会话信息
 *          tcp_head           tcp数据
 *			client_or_server   数据方向
 *输出参数：none
 *返回值：  none
*/
void tcp_session_rst(struct tcp_session *session, struct tcphdr *tcp_head, short client_or_server);

/*
 *函数名称：tcp_session_psh
 *函数功能：处理psh数据包
 *输入参数：session            tcp session
 *          pkt_info           数据包的信息
 *          tcp_head           tcp数据
 *          thl                tcp头的大小
 *          tcp_data_size      tcp数据的大小
 *          hash_index         hash索引
 *输出参数：none
 *返回值：  none
*/
void tcp_session_psh(struct tcp_session *session,struct packet_info *pkt_info,struct tcphdr *tcp_head,
		             int thl,int tcp_data_size,int hash_index);

/*
 *函数名称：tcp_session_fin
 *函数功能：处理第一次与第三次挥手
 *输入参数：session            tcp session
 *          tcp_head           tcp头
 *			client_or_server   数据方向
 *输出参数：none
 *返回值：  none
*/
void tcp_session_fin(struct tcp_session *session, struct tcphdr *tcp_head, 
		             short client_or_server);

/*
 *函数名称：tcp_session_fin_ack
 *函数功能：处理第二次与第四次挥手
 *输入参数：session            tcp session
 *			pkt_info           数据包信息
 *          tcp_head           tcp头
 *          tcp_data_size      tcp数据的大小
 *          thl                tcp head size
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_fin_ack(struct tcp_session *session, struct packet_info *pkt_info, 
					    struct tcphdr *tcp_head,unsigned tcp_data_size, int thl);

/*
 *函数名称：tcp_session_process
 *函数功能：对外的tcp组包函数
 *输入参数：session            tcp session
 *          pkt_info           数据包信息
 *          tcp_head           tcp数据
 *          tcp_data_size      tcp数据的的大小
 *          thl                tcp 头部大小
 *输出参数：none
 *返回值：  none
*/
void tcp_session_process(struct tcp_session *session,struct packet_info *pkt_info, struct tcphdr *tcp_head, 
		                 unsigned tcp_data_size, int thl);

#endif  //_TCP_SESSION_H_

