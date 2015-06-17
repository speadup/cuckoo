/***********************************************************
 *文件名：tcp_session.c
 *创建人：小菜_默
 *日  期：2013年11月06日
 *描  述：tcp组包的实现部分
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#include "tcp_session.h"
#include "tcp_check_time.h"
#include "stdyeetec.h"

#include "net_defs.h"
#include "hash.h"
#include "config.h"
#include "mempool.h"
#include "data_transit.h"
#include "net_debug.h"

//大小端转换头函数使用的文件
#include <arpa/inet.h>
#include <linux/tcp.h>

//表示tcp链接的状态
enum
{
	TCP_ESTABLISHED = 1,
	TCP_SYN_SENT,
	TCP_SYN_RECV,
	TCP_FIN_WAIT1,
	TCP_FIN_WAIT2,
	TCP_TIME_WAIT,
	TCP_CLOSE,
	TCP_CLOSE_WAIT,
	TCP_LAST_ACK,
	TCP_LISTEN,
	TCP_CLOSING   /* now a valid state */
};

int is_early_packet(struct tcp_session *session,struct tcphdr *tcp_head, unsigned tcp_data_size, int thl,unsigned seq);

int tcp_session_merge(struct tcp_session *session,struct tcphdr *tcp_head, 
		              int thl, unsigned tcp_data_size,short client_or_server);
int tcp_session_early(struct tcp_session *session, struct packet_info *pkt_info, 
		              short client_or_server);
void tcp_session_merge_free(struct tcp_session *session);

struct session_ring *gtcp_session_ring = NULL;
static unsigned gsession_max_links = 0;  //为了解决部分hash冲突，定义了组包内部的最大连接数
static unsigned hash_max_list = 0;		 //定义hash链表最大个数
static unsigned session_max_num = 0;	 //传进来的最大会话数
atomic_t gtcp_session_count;

struct hash_index{
	struct list_head hash_head;
	int hash_num;
};

/*
 *函数名称：tcp_session_init
 *函数功能：对组包模块进行初始化
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_init()
{
	INFO_PRINT("Net->Tcp->Session模块初始化\n");

	atomic_set(&gtcp_session_count,0);

	struct yt_config *config = base_get_configuration();
	assert (config);

	//减少hash冲突内部最大链接是比规定连接数多1/4
	session_max_num = config->number_of_links;
	gsession_max_links = config->number_of_links * 4/3;
	INFO_PRINT("tcp session max links (number_of_links * 4/3) = %d\n",gsession_max_links);

	hash_max_list = config->hash_max_list;
	INFO_PRINT("tcp session hash list max num = %d\n", hash_max_list);

	gtcp_session_ring = malloc(gsession_max_links * sizeof(struct session_ring));
	if(!gtcp_session_ring){
		log_write(LOG_ERROR, "Net->Tcp->Session gtcp_session_ring malloc error\n");
		return YT_FAILED;
	}

	//初始化所有链接的头
	int i = 0;

	for(i=0;i<gsession_max_links;i++) {
		INIT_LIST_HEAD(&(gtcp_session_ring[i].session_head));
		pthread_spin_init(&(gtcp_session_ring[i].spin_lock),0);
		atomic_set(&(gtcp_session_ring[i].hash_max_num), 0);
	}
	
	if (YT_FAILED == tcp_check_time_init(gtcp_session_ring,gsession_max_links)) {
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
 *函数名称：tcp_session_destroy
 *函数功能：停止模块工作释放该模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_session_destroy()
{	
	INFO_PRINT("Net->Tcp->Session模块释放\n");

	tcp_check_time_destroy();

	int i = 0;
	for (i=0;i<gsession_max_links;i++) {
		//没有释放会话链表中的内存
		pthread_spin_destroy(&gtcp_session_ring[i].spin_lock);
	}

	if(gtcp_session_ring){
		free(gtcp_session_ring);
		gtcp_session_ring  = 0;
	}
}

/*
 *函数名称：tcp_session_find
 *函数功能：查找tcp链接是否存在
 *输入参数：element_data       元组数据
 *          tcp_data           tcp数据
 *输出参数：hash               返回hash值
 *返回值：  找到数据返回，查找失败返回NULL
*/
struct tcp_session *tcp_session_find(struct packet_info *pkt_info, struct tcphdr *tcp_data,
									 int thl,int tcp_data_size,unsigned *hash)
{
	//返回结构体后，多线程中可以修改seq、ack和status等信息的前提是要保证一个会话
	//只有一个线程处理。在get_packet模块中实现。
	struct tcp_session *session_tmp = NULL;
	struct list_head *zip_node      = NULL;
	struct net_tuple tuple_tmp;
	memset(&tuple_tmp,0,sizeof(struct net_tuple));

	//按照反方向赋值tuple元素
	memcpy(tuple_tmp.ip6_saddr,pkt_info->tuple.ip6_daddr,IP6_LEN);
	memcpy(tuple_tmp.ip6_daddr,pkt_info->tuple.ip6_saddr,IP6_LEN);
	tuple_tmp.ip4_saddr   = pkt_info->tuple.ip4_daddr;
	tuple_tmp.ip4_daddr   = pkt_info->tuple.ip4_saddr;
	tuple_tmp.port_source = pkt_info->tuple.port_dest;
	tuple_tmp.port_dest   = pkt_info->tuple.port_source; 

	//赋值四层的协议类型，区分tcp，udp等协议
	tuple_tmp.l4_type = pkt_info->tuple.l4_type;

	//源ip和端口号按照相反的顺序hash
	*hash = mkhash (tuple_tmp.ip4_saddr, tuple_tmp.ip6_saddr, tuple_tmp.port_source,
			        tuple_tmp.ip4_daddr, tuple_tmp.ip6_daddr, tuple_tmp.port_dest) %
		        gsession_max_links;

	TCP_PRINT("第一次 hash_index :%u,seq:%u ack:%u\n",
			  *hash,ntohl(tcp_data->seq),ntohl(tcp_data->ack_seq));

	pthread_spin_lock(&gtcp_session_ring[*hash].spin_lock);
	{
		//遍历hash冲突连
		list_for_each(zip_node, &gtcp_session_ring[*hash].session_head) {
			//zip_nod 在结构体的头，所以zip_node的地址就是结构体的地址
			session_tmp = (struct tcp_session*)zip_node;
			TCP_PRINT("*****************服务器数据*********************************\n");

			if (!memcmp(((unsigned char*)(&session_tmp->tuple))+SKIP_MAC, 
						((unsigned char*)&tuple_tmp)+SKIP_MAC, 
						sizeof(struct net_tuple)-SKIP_MAC)) {
				pkt_info->client_or_server = FROM_SERVER_DATA;

				mempool_free (session_tmp->pkt_info);
				session_tmp->pkt_info = pkt_info;

				if (is_early_packet(session_tmp,tcp_data,tcp_data_size,thl,session_tmp->seq[FROM_SERVER_DATA])) {
//		DEBUG_PRINT ("-------------seq:%u,ack:%u\n\n",ntohl(tcp_data->seq),ntohl(tcp_data->ack_seq));
					return NULL;
				}

				//该处返回不进行锁的释放，返回后在tcp.c部分进行释放
				return session_tmp;
			}
		}
	}
	pthread_spin_unlock(&gtcp_session_ring[*hash].spin_lock);
	
	//源ip和端口号按照正常的顺序hash
	*hash = mkhash (pkt_info->tuple.ip4_saddr  ,pkt_info->tuple.ip6_saddr,
	                pkt_info->tuple.port_source,pkt_info->tuple.ip4_daddr,
					pkt_info->tuple.ip6_daddr  ,pkt_info->tuple.port_dest) %
		         gsession_max_links;

	TCP_PRINT("第二次 hash_index :%u\n",*hash);

	pthread_spin_lock(&gtcp_session_ring[*hash].spin_lock);
	{
		list_for_each(zip_node, &gtcp_session_ring[*hash].session_head) {
			session_tmp = (struct tcp_session*)zip_node;
			TCP_PRINT("*******************客户端数据*******************************\n");

			if (!memcmp(((unsigned char*)(&session_tmp->tuple))+SKIP_MAC, 
						((unsigned char*)(&pkt_info->tuple))+SKIP_MAC, 
						sizeof(struct net_tuple)-SKIP_MAC)) {

				pkt_info->client_or_server = FROM_CLIENT_DATA;

				mempool_free (session_tmp->pkt_info);
				session_tmp->pkt_info = pkt_info;

				if (is_early_packet(session_tmp,tcp_data,tcp_data_size,thl,session_tmp->seq[FROM_CLIENT_DATA])) {
					return NULL;
				}

				//该处返回不进行锁的释放，返回后在tcp.c部分进行释放
				return session_tmp;
			}
		}
	}
	//该处返回不进行锁的释放，返回后在tcp.c部分进行释放

	pkt_info->client_or_server = FROM_NONE_DATA;

	return session_tmp;
}

/*
 *函数名称：tcp_session_first
 *函数功能：处理tcp的第一次握手
 *输入参数：session            tcp session
 *          pkt_info           包信息
 *          tcp_head           tcp数据
 *          hash               该包对应的hash value
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_first(struct packet_info *pkt_info,struct tcphdr *tcp_head,
		              unsigned hash)
{
	//链接数判断，若大于规定的数量直接返回
	if(atomic_read(&gtcp_session_count) >= session_max_num){
		return YT_FAILED;
	}

	//超过最大hash数
	if(atomic_read(&(gtcp_session_ring[hash].hash_max_num)) >= hash_max_list){
		return YT_FAILED;
	}

	//若没有找到就分配内存赋值并返回
	struct tcp_session *session_tmp = mempool_malloc(sizeof(struct tcp_session));
	if (!session_tmp) {
		log_write(LOG_ERROR,"Net->Tcp->Merge模块内存池获取内存分配失败[%d line].\n",__LINE__);
		return YT_FAILED;
	}

	session_tmp->deal_record = mempool_malloc(sizeof(struct session_deal_record));
	if(!(session_tmp->deal_record)){
		log_write(LOG_ERROR,"Net->Tcp->deal_record内存分配失败[%d line].\n",__LINE__);
		return YT_FAILED;
	}

	session_tmp->pkt_info = pkt_info;
	session_tmp->hash_index = hash;

	//初始化新链接数据与状态
	INIT_LIST_HEAD(&session_tmp->tcp_early_head);
	session_tmp->seq[FROM_CLIENT_DATA]    = ntohl(tcp_head->seq) + 1;
	session_tmp->status[FROM_CLIENT_DATA] = TCP_SYN_SENT;
	session_tmp->status[FROM_SERVER_DATA] = TCP_CLOSE;

	session_tmp->time  = time (0);
	session_tmp->tuple = pkt_info->tuple;

	(session_tmp->deal_record)->session_status = SESSION_STATUS_RECV;
	(session_tmp->deal_record)->session_index = NULL;

	
	//tcp_links ++;       //链接数 +1
	list_add_tail(&session_tmp->zip_node, &(gtcp_session_ring[hash].session_head));
	
	atomic_inc(&(gtcp_session_ring[hash].hash_max_num));
	atomic_inc(&gtcp_session_count);
	TCP_PRINT ("+++++++++++session malloc count: %u++++++++++++++++++\n",atomic_read(&gtcp_session_count));
	TCP_PRINT("***********************Tcp会话第一次握手**********************\n");
	
	return YT_SUCCESSFUL;
}

/*
 *函数名称：tcp_session_second
 *函数功能：tcp的第二次握手
 *输入参数：session            tcp session
 *          tcp_head           tcp数据
 *输出参数：none
 *返回值：  none
*/
void tcp_session_second(struct tcp_session *session,struct tcphdr *tcp_head)
{
	//数据方向 server -> client
	if(UNLIKELY(TCP_SYN_SENT != session->status[FROM_CLIENT_DATA]||
	            TCP_CLOSE != session->status[FROM_SERVER_DATA])) {
		return ;
	}

	//第二次回应包的ACK 值为第一个包的序列号+1,在初始化的时候已经+1
	if(UNLIKELY(ntohl(tcp_head->ack_seq) != session->seq[FROM_CLIENT_DATA])) {
		return ;
	}

	TCP_PRINT("*********************Tcp会话第二次握手**********************\n");
	session->status[FROM_SERVER_DATA] = TCP_SYN_RECV;
	session->seq[FROM_SERVER_DATA]    = ntohl(tcp_head->seq) + 1;
	session->ack_seq[FROM_SERVER_DATA]= ntohl(tcp_head->ack_seq);
	session->time                     = time (0);

	return ;
}

/*
 *函数名称：tcp_session_rst
 *函数功能：处理rst数据包
 *输入参数：session            会话信息
 *          tcp_head           tcp数据
 *			client_or_server   数据方向
 *输出参数：none
 *返回值：  none
*/
void tcp_session_rst(struct tcp_session *session, struct tcphdr *tcp_head, short client_or_server)
{
	//rst包可能在第二次握手之后，也可能在第三握手之后，或者其他。
	//rst包在第三次挥手之后,相当与是低四次挥手
  	if (UNLIKELY(TCP_CLOSE_WAIT== session->status[!client_or_server] &&
				 TCP_TIME_WAIT == session->status[client_or_server])) {

   		if (LIKELY(ntohl(tcp_head->ack_seq) != session->seq[!client_or_server]-1)) {
			return ;
   		}

		TCP_PRINT("**********第三次次挥手之后的Rst数据包处理********************\n");
		goto rst_session_free;
   	}

	//第二次或其他
	if (ntohl(tcp_head->ack_seq) == session->seq[!client_or_server]) {

		TCP_PRINT("**************Rst数据包处理********************\n");
		//tcp_session_free(session);
	}

//	unsigned hash_index = session->hash_index;
rst_session_free:
	data_transit_put(session->deal_record,session->pkt_info,session->data_buffer,session->buf_size,SESSION_STATUS_CLOSE);
	tcp_session_free(session);
}

/*
 *函数名称：tcp_session_third
 *函数功能：处理tcp的第三次握手
 *输入参数：session            tcp session
 *          tcp_head           tcp数据
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_third(struct tcp_session *session,struct tcphdr *tcp_head)
{
	//数据来源 client -> server
	if(UNLIKELY(TCP_SYN_SENT != session->status[FROM_CLIENT_DATA] ||
	            TCP_SYN_RECV != session->status[FROM_SERVER_DATA])) {
		return YT_FAILED;
	}

	if(ntohl(tcp_head->ack_seq) == session->seq[FROM_SERVER_DATA]){
		//链接建立成功，更新客户端与服务器端的状态
		session->status[FROM_CLIENT_DATA] = TCP_ESTABLISHED;
		session->status[FROM_SERVER_DATA] = TCP_ESTABLISHED;
		session->ack_seq[FROM_CLIENT_DATA]= ntohl(tcp_head->ack_seq);
		session->time                     = time(0);

		TCP_PRINT("**************Tcp会话第三次握手**********************\n");
	}

	return YT_SUCCESSFUL;
}

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
void tcp_session_psh(struct tcp_session *session,struct packet_info *pkt_info,struct tcphdr *tcp_head,int thl,
		           int tcp_data_size,int hash_index)
{
	short client_or_server = pkt_info->client_or_server;

	//判断seq与ack是否正确
	if (LIKELY(ntohl(tcp_head->seq) != session->seq[client_or_server])) {
		return ;
	}
	
	//判断是否为第二次挥手包
	if (UNLIKELY(TCP_FIN_WAIT1 == session->status[!client_or_server] && 
				 TCP_CLOSE_WAIT == session->status[client_or_server])) {

		TCP_PRINT("********************第二次挥手包处理******************************\n");
		session->status[!client_or_server]  = TCP_FIN_WAIT2;
	}

	//更改本次的seq和对端的ack
	session->seq[client_or_server]    += tcp_data_size;
	session->ack_seq[!client_or_server]= session->seq[client_or_server];
	session->time = time(0);

	TCP_PRINT("******Psh数据包处理*******data size :%d *** session seq: %u ********\n",
			  tcp_data_size,session->seq[client_or_server]);

	char *psh_data = (char*)mempool_malloc(tcp_data_size + 1);
	if(!psh_data){
		log_write(LOG_ERROR, "psh mempool_malloc error\n");
		return ;
	}

	memcpy(psh_data, (char*)tcp_head+thl, tcp_data_size);

	data_transit_put(session->deal_record, session->pkt_info, psh_data, tcp_data_size, SESSION_STATUS_RECV);
	
	return ;
}

/*
 *函数名称：tcp_session_fin
 *函数功能：处理第一次与第三次挥手
 *输入参数：session            tcp session
 *          tcp_head           tcp头
 *			client_or_server   数据方向
 *输出参数：none
 *返回值：  none
*/
void tcp_session_fin (struct tcp_session *session, struct tcphdr *tcp_head, 
		              short client_or_server)
{
	//判断是否是第一次挥手
   	if (TCP_ESTABLISHED == session->status[client_or_server]) {

		//第一次挥手 
   		if (LIKELY(ntohl(tcp_head->ack_seq) != session->seq[!client_or_server])) {
			return ;
   		}

		TCP_PRINT("********************第一次挥手包处理******************************\n");
		session->status[client_or_server]  = TCP_FIN_WAIT1;
		session->status[!client_or_server] = TCP_CLOSE_WAIT;
		session->seq[client_or_server]    += 1;
		session->ack_seq[!client_or_server]= session->seq[client_or_server];
		session->time = time(0);

		return ;
   	}

	//判断是否是第三次挥手
   	if (TCP_FIN_WAIT2  == session->status[!client_or_server] &&
   		TCP_CLOSE_WAIT == session->status[client_or_server]) {

		//第三次挥手
   		if (LIKELY(ntohl(tcp_head->ack_seq) != session->seq[!client_or_server])) {
			return ;
   		}

		TCP_PRINT("********************第三次挥手包处理******************************\n");
		session->status[!client_or_server] = TCP_TIME_WAIT;
		session->seq[client_or_server]    += 1;
		session->ack_seq[!client_or_server]= session->seq[client_or_server];
		session->time = time(0);
		return ;
   	}
   
	return ;
}

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
int tcp_session_fin_ack (struct tcp_session *session, struct packet_info *pkt_info, 
						  struct tcphdr *tcp_head,unsigned tcp_data_size, int thl)
{
	short client_or_server = pkt_info->client_or_server;

	//判断是否为第二次挥手包
	if (UNLIKELY(TCP_FIN_WAIT1 == session->status[!client_or_server] && 
				 TCP_CLOSE_WAIT == session->status[client_or_server])) {

		//第二次挥手
  		if (LIKELY(ntohl(tcp_head->ack_seq) != session->seq[!client_or_server])) {
			return YT_SUCCESSFUL;
		}

		TCP_PRINT("********************第二次挥手包处理******************************\n");
		session->status[!client_or_server]  = TCP_FIN_WAIT2;
		session->time = time(0);
		return YT_SUCCESSFUL;
	}

	//判断是否为第四次挥手包
	if (UNLIKELY(TCP_CLOSE_WAIT== session->status[!client_or_server] &&
				 TCP_TIME_WAIT == session->status[client_or_server])) {

		//第四次挥手
  		if (LIKELY(ntohl(tcp_head->ack_seq) != session->seq[!client_or_server])) {
			return YT_SUCCESSFUL;
		}

		//session->status[!client_or_server] = TCP_LAST_ACK;
		session->time = time(0);
		
		TCP_PRINT("********************第四次挥手包处理******************************\n");
		data_transit_put(session->deal_record,session->pkt_info,NULL,0,SESSION_STATUS_CLOSE);
		//释放会话
		tcp_session_free(session);

		return YT_SUCCESSFUL;
	}
		
	return YT_FAILED;
}

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
void tcp_session_process(struct tcp_session *session, struct packet_info *pkt_info,
		                 struct tcphdr *tcp_head, unsigned tcp_data_size, int thl)
{
	assert (pkt_info);

	//来数据后，没有判断会话的状态，某端会话可能已经关闭，但还有数据到来。
	int client_or_server = session->pkt_info->client_or_server;
	if (LIKELY(YT_SUCCESSFUL == tcp_session_merge(session,tcp_head,thl,tcp_data_size,
					                              client_or_server))) {
		//记录包大小 疑问：流量统计是否统计了tcp ip头等的大小
		//session->pkt_size += pkt_info->pkt_size;

		if (UNLIKELY(tcp_head->fin)) {
			TCP_PRINT("********************第一次挥手包处理******************************\n");
			session->status[client_or_server]  = TCP_FIN_WAIT1;
			session->status[!client_or_server] = TCP_CLOSE_WAIT;
			session->seq[client_or_server]    += 1;
			session->ack_seq[!client_or_server]= session->seq[client_or_server];
			session->time = time(0);
			//该处不家return 因为是fin包之后，还可能是psh包，所以要继续往下判断。
			//return ;
		}

		//判断psh包
		if (UNLIKELY(tcp_head->psh)) {
			session->time = time(0);

			TCP_PRINT("*******************Psh数据包处理******************\n");
			//组包完成，数据分段。
			data_transit_put(session->deal_record,session->pkt_info, session->data_buffer,session->buf_size,SESSION_STATUS_RECV);
			
			//释放相应的数据  buf 和 early
		//	tcp_session_merge_free(session);
			
			session->buf_size = 0;
			session->data_buffer = NULL;

			return ;
		}

		//该处while会做调整，在该处将会去掉
		//遍历提前到的包队列，判断是否有可以继续组包
		while(YT_SUCCESSFUL == tcp_session_early(session, session->pkt_info, client_or_server));

		return ;
	}

	return ;
}

int is_early_packet(struct tcp_session *session, struct tcphdr *tcp_head, unsigned tcp_data_size, int thl,unsigned seq)
{
	//提前来到的包
	if (session->status[FROM_SERVER_DATA] != TCP_CLOSE && UNLIKELY(ntohl(tcp_head->seq) > seq)) {

		struct tcp_early_packet *early_packet = mempool_malloc(sizeof(struct tcp_early_packet));
		if (!early_packet) {
			log_write(LOG_ERROR,"Net->Tcp->Session模块:%d line 内存池获取内存分配失败.\n",__LINE__);
			return YT_IS_TRUE;
		}

		early_packet->tcp_head = mempool_malloc(thl+tcp_data_size+1);
		if (!early_packet->tcp_head) {
			log_write(LOG_ERROR,"Net->Tcp->Session模块:%d line 内存池获取内存分配失败.\n",__LINE__);
			return YT_IS_TRUE;
		}
		memcpy(early_packet->tcp_head,tcp_head,thl+tcp_data_size+1);

		early_packet->thl           = thl;
		early_packet->tcp_data_size = tcp_data_size;
		session->time               = time(0);

		list_add(&early_packet->info_node, &session->tcp_early_head);

		return YT_IS_TRUE;
	}


	return YT_IS_FALSE;
}

/*
 *函数名称：tcp_session_merge
 *函数功能：tcp组包函数
 *输入参数：session            tcp session
 *          tcp_head           tcp数据
 *          thl                tcp 头部大小
 *          tcp_data_size      tcp数据的的大小
 *          client_or_server   数据方向
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_merge(struct tcp_session *session,struct tcphdr *tcp_head, 
		              int thl, unsigned tcp_data_size,short client_or_server)
{
	TIME_BEGIN(1);

	int ret_value = YT_FAILED;
	
	//顺序来的包
	if (ntohl(tcp_head->seq) == session->seq[client_or_server]) {
		if (LIKELY(ntohl(tcp_head->ack_seq) == session->ack_seq[client_or_server])) {

			//分配空间，并进行数据拼接,并更新时间
			session->data_buffer = mempool_realloc(session->data_buffer,session->buf_size+tcp_data_size);
			if (!session->data_buffer) {
				log_write(LOG_ERROR,"Net->Tcp->Session模块:%d line 内存池获取内存分配失败.\n",__LINE__);

				ret_value = YT_FAILED;
				goto ret_session_merge;
			}

//			mempool_memory_info(session->data_buffer,"session->data_buffer");
//			DEBUG_PRINT("SPORT:%d\n",ntohs(tcp_head->source));
//			DEBUG_PRINT("DPORT:%d\n",ntohs(tcp_head->dest));

			memcpy(session->data_buffer+session->buf_size,((unsigned char*)tcp_head)+thl,tcp_data_size);
			session->buf_size              += tcp_data_size;
			session->seq[client_or_server] += tcp_data_size; 
			session->ack_seq[!client_or_server] = session->seq[client_or_server];
			session->time = time(0);

			ret_value = YT_SUCCESSFUL;
			goto ret_session_merge;
		}
	}

ret_session_merge:

	TIME_END(1);
	return ret_value;
}

/*
 *函数名称：tcp_session_early
 *函数功能：处理tcp提前到的数据包
 *输入参数：session            tcp session
 *			pkt_info           数据包信息
 *          client_or_server   数据方向
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_session_early(struct tcp_session *session, struct packet_info *pkt_info,
		              short client_or_server)
{
	struct tcp_early_packet *early_packet = NULL;
	struct list_head *pos     = NULL;
	struct list_head *pos_tmp = NULL;
	int ret = YT_SUCCESSFUL;
	
	//需要倒着遍历tcp_early_packet结构体进行组包
	list_for_each_prev_safe(pos, pos_tmp, &(session->tcp_early_head)) {
		early_packet = (struct tcp_early_packet*)pos;

		if (LIKELY(YT_SUCCESSFUL == tcp_session_merge(session,early_packet->tcp_head,
						                              early_packet->thl,
													  early_packet->tcp_data_size,
													  client_or_server))) {
			//组包成功要删除该节点数据
			list_del(pos);

			if (UNLIKELY(early_packet->tcp_head->fin)) {
				session->status[client_or_server]  = TCP_FIN_WAIT1;
				session->seq[client_or_server]    += 1;
				session->ack_seq[!client_or_server]= session->seq[client_or_server];
			}

			//判断psh包
			if (UNLIKELY(early_packet->tcp_head->psh)) {
				//组包完成，数据分段。
				data_transit_put(session->deal_record,session->pkt_info,session->data_buffer,session->buf_size,SESSION_STATUS_RECV); 
		
				//释放相应的数据  buf 和 early
//				tcp_session_merge_free(session);

				session->data_buffer = NULL;
				session->buf_size = 0;
			
				ret = YT_FAILED;
				goto ret_free_data;
			}
	
			//该步操作可能会与本次循环发生冲突，为了减少该次操作，外层加了一个while
			//tcp_session_early(session, pkt_info, client_or_server);

			ret_free_data:	
			mempool_free (early_packet->tcp_head);
			early_packet->tcp_head = NULL;

			mempool_free (pos);
			pos = NULL;

			return ret;
		}
	}

	return YT_FAILED;
}

/*
 *函数名称：tcp_session_merge_free
 *函数功能：释放组包
 *输入参数：session  tcp session
 *输出参数：none
 *返回值：  none
*/
void tcp_session_merge_free(struct tcp_session *session)
{
	assert (session);
	struct list_head *pos,*pos_tmp = pos = NULL;
	TCP_PRINT("=============TCP 组包释放=========== %lu\n",pthread_self());

	list_for_each_safe(pos,pos_tmp,&(session->tcp_early_head)) {
		list_del(pos);

		mempool_free(((struct tcp_early_packet*)pos)->tcp_head);
		((struct tcp_early_packet*)pos)->tcp_head = NULL;

		mempool_free(pos);
		pos = NULL;
	}

//	mempool_free(session->data_buffer);
	session->data_buffer = NULL;

	session->buf_size = 0;
	session->pkt_size = 0;
	
}

/*
 *函数名称：tcp_session_free
 *函数功能：释放会话
 *说    明：该函数的定义在 tcp_check_time.h 文件中
 *输入参数：session            tcp session
 *输出参数：none
 *返回值：  none
*/
void tcp_session_free(struct tcp_session *session)
{
	assert (session);
	tcp_session_merge_free(session);

	mempool_free(session->pkt_info);
	session->pkt_info = NULL;
	
	list_del((struct list_head*)session);

	atomic_dec(&gtcp_session_count);
	atomic_dec(&(gtcp_session_ring[session->hash_index].hash_max_num));
	
	mempool_free(session);
	session = NULL;
}


