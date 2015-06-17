/***********************************************************
 *文件名：tcp.c
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：tcp数据处理的实现部分
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "tcp.h"
#include "stdyeetec.h"

#include "tcp_session.h"
#include "net_defs.h"
#include "mempool.h"
#include "net_debug.h"
#include "tcp_check_time.h"
#include "atomic.h"

#include <pthread.h>
#include <linux/tcp.h>
#include <arpa/inet.h>  

inline void tcp_data_process(struct packet_info *pkt_info,struct tcphdr *tcp_head,int thl,
		                     int tcp_data_size);

extern struct session_ring *gtcp_session_ring;  //tcp_session.c

static atomic_t gsession_counter;
static atomic_t gpsh_counter;
static atomic_t gdrop_counter;

/*
 *函数名称：tcp_init
 *函数功能：完成tcp模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_init()
{
	INFO_PRINT("Net->Tcp模块初始化\n");

	atomic_set(&gsession_counter,0);
	atomic_set(&gpsh_counter,0);
	atomic_set(&gdrop_counter,0);

	return tcp_session_init();
}

/*
 *函数名称：tcp_destroy
 *函数功能：释放tcp模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_destroy()
{
	tcp_session_destroy();
	INFO_PRINT("Net->Tcp模块释放\n");
}

/*
 *函数名称：tcp_input
 *函数功能：处理tcp数据
 *输入参数：pkt_info           数据包信息
 *          tcp                tcp数据
 *          tcp_size           tcp数据的的大小
 *输出参数：none
 *返回值：  none
*/
void tcp_input(struct packet_info *pkt_info, const char *tcp, const unsigned tcp_size)
{
	struct tcphdr *tcp_head = (struct tcphdr*)tcp;
	int thl = TCP_HEAD_LEN(tcp_head->doff);
	int tcp_data_size = tcp_size - thl;

    pkt_info->tuple.l4_type     = YT_IS_TCP;
	pkt_info->tuple.port_source = tcp_head->source;
	pkt_info->tuple.port_dest   = tcp_head->dest;

	TCP_PRINT_PORT("SPORT",ntohs(tcp_head->source));
	TCP_PRINT_PORT("DPORT",ntohs(tcp_head->dest));

	tcp_data_process(pkt_info,tcp_head,thl,tcp_data_size);
	return ;
}

/*
 *函数名称：tcp_data_process
 *函数功能：处理tcp数据
 *输入参数：pkt_info           数据包信息
 *          tcp_head           tcp头
 *          thl                tcp头大小
 *          tcp_data_size      tcp 数据长度
 *输出参数：none
 *返回值：  none
*/
inline void tcp_data_process(struct packet_info *pkt_info,struct tcphdr *tcp_head,int thl,
		                     int tcp_data_size)
{
	TCP_PRINT("**********data size %d ****** head size %d ********\n",tcp_data_size,thl);

	unsigned hash_index = 0;
	struct tcp_session *session = tcp_session_find(pkt_info,tcp_head,thl,tcp_data_size,&hash_index);
	if(FROM_NONE_DATA == pkt_info->client_or_server){
		//若没有找到，添加新连接,处理第一个握手包.
		//第一次握手 tcp数据包中syn==1 && ack==0 && rst == 0 时,数据方向 client -> server。
		if(tcp_head->syn && !tcp_head->ack && !tcp_head->rst){
			tcp_session_first(pkt_info,tcp_head,hash_index);
		}
		else {
//		yt_atomic32_inc(&gdrop_counter);
//		if (yt_atomic32_read(&gdrop_counter) %1 == 0) {
//			DEBUG_PRINT ("++++Drop count: %u++ Hash: %u ++++seq:%u +++\n",
//			yt_atomic32_read(&gdrop_counter),hash_index,ntohl(tcp_head->seq));

//			IPv4_PRINT_IP ("From Sip",&pkt_info->tuple.ip4_saddr);
//			IPv4_PRINT_IP ("From Dip",&pkt_info->tuple.ip4_daddr);
//		}

			//若不是第一次握手包就进行释放,否则会内存泄露
			mempool_free(pkt_info);
		//	mempool_free_my(0, pkt_info);
			pkt_info = NULL;
		}
		goto return_tcp_process;
	}

	//session 为空，说明是提前来到的包，该判定必须在上个if的后面
	if (!session) {
		goto return_tcp_process;
	}

	short client_or_server = pkt_info->client_or_server;

	//第二次握手 tcp数据包中syn==1 && ack==1 ，数据方向 server -> client
	if(tcp_head->syn && tcp_head->ack && FROM_SERVER_DATA == client_or_server){
		tcp_session_second(session,tcp_head);
		goto return_tcp_process;
	}
	
	//第三次握手 ack == 1 数据方向是 client -> server
	if(LIKELY(tcp_head->ack && FROM_CLIENT_DATA == client_or_server)){
		//带有ack的包不一定是第三次握手，不是第三次握手需继续往下走
		if (YT_SUCCESSFUL == tcp_session_third(session,tcp_head)) {
			goto return_tcp_process;
		}
	}

	//数据包分段处理,有些挥手包中会出现psh和fin同为1的情况
	//data_buffer不为空，说明是要组包的数据,组包的最后一个包是带有psh的
	if(tcp_head->psh && !tcp_head->fin && !session->data_buffer){
		tcp_session_psh(session,pkt_info,tcp_head,thl,tcp_data_size,session->hash_index);

//		yt_atomic32_inc(&gpsh_counter);
//		if (yt_atomic32_read(&gpsh_counter) %10000 == 0) {
//			DEBUG_PRINT ("+++++++++++psh count: %u++++++++++++++++++\n",yt_atomic32_read(&gpsh_counter));
//		}

		goto return_tcp_process;
	}
	
	//处理rst包 rst == 1
	if(tcp_head->rst){
		tcp_session_rst(session, tcp_head, client_or_server);
		goto return_tcp_process ;
	}

	//处理第一次、第三次挥手包
	if (tcp_head->fin && 0 == tcp_data_size) {
		tcp_session_fin (session, tcp_head, client_or_server);
		goto return_tcp_process;
	}

	//第二次与第四次挥手处理
	if(LIKELY(tcp_head->ack)){
		//带有ack的包不一定都是的二次与第四次挥手包，所以需要判断
		if (YT_SUCCESSFUL == tcp_session_fin_ack (session, pkt_info, tcp_head, tcp_data_size, thl)) {
			goto return_tcp_process;
		}
	}

	//删除窗口协商的数据的包,下面的数据处理是需要处理psh的
	if (UNLIKELY(tcp_data_size <= 0)) {
		//没有对窗口协商的数据包进行大小统计
		goto return_tcp_process;
	}

	//数据包和第一次挥手处理,这儿不需要if判断，前面的很多判断已经把数据过滤完成
	tcp_session_process(session,pkt_info,tcp_head,tcp_data_size,thl); 

	//处理完成后解锁返回
	pthread_spin_unlock(&gtcp_session_ring[hash_index].spin_lock);

//	yt_atomic32_inc(&gsession_counter);
//	if (yt_atomic32_read(&gsession_counter) %10000 == 0) {
//		DEBUG_PRINT ("+++++++++++packet count: %u++++++++++++++++++\n",yt_atomic32_read(&gsession_counter));
//	}

	return ;

return_tcp_process:
	//解锁返回
	pthread_spin_unlock(&gtcp_session_ring[hash_index].spin_lock);

//	yt_atomic32_inc(&gsession_counter);
//	if (yt_atomic32_read(&gsession_counter) %10000 == 0) {
//		DEBUG_PRINT ("+++++++++++packet count: %u++++++++++++++++++\n",yt_atomic32_read(&gsession_counter));
//	}
}
