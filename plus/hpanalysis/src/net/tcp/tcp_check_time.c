/***********************************************************
 *文件名：tcp_check_time.c
 *创建人：小菜_默
 *日  期：2013年11月06日
 *描  述：tcp组包中的超时检测
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#include "tcp_check_time.h"
#include "stdyeetec.h"

#include "config.h"
#include "pthread_affinity.h"
#include "data_transit.h"
#include "net_debug.h"

#include <unistd.h>

void *tcp_check_time_thread (void *arg);

void tcp_check_time(struct yt_config *config, struct session_ring *tcp_session_ring);
//全局的会话链表
static struct session_ring *gtcp_session_ring = NULL;

static unsigned gsession_max_links = 0; //全局最大会话数量
static pthread_t gthread_id        = 0; //超时检测线程的id号

/*
 *函数名称：tcp_check_time_init
 *函数功能：初始化tcp超时检测模块
 *输入参数：tcp_session_ring   会话链表
 *          session_max_links  会话的最大链接数
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int tcp_check_time_init(struct session_ring *tcp_session_ring,unsigned session_max_links)
{
	INFO_PRINT("Net->Tcp->Session->Check Time模块初始化\n");

	gtcp_session_ring  = tcp_session_ring;
	gsession_max_links = session_max_links;

	if (YT_SUCCESSFUL != pthread_create(&gthread_id,NULL,tcp_check_time_thread,NULL)) {
		log_write(LOG_ERROR,"Net->Tcp->Session->Check Time模块:%d line 线程启动失败.\n",__LINE__);
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
 *函数名称：tcp_check_time_destroy
 *函数功能：释放函数
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_check_time_destroy()
{
	INFO_PRINT("Net->Tcp->Session->Check Time模块释放\n");
	pthread_join(gthread_id,NULL);
}

/*
 *函数名称：tcp_check_time_thread
 *函数功能：tcp 会话超时检测线程
 *输入参数：arg       线程输入参数
 *输出参数：none
 *返回值：  NULL 
*/
void *tcp_check_time_thread (void *arg)
{
	int i = 0;

	struct yt_config *config = base_get_configuration();
	assert (config);

	//查找使用的哪个cpu
	for (i=0;i<YT_MAX_LCORE;i++) {
		//判断并设定线程到指定cpu
		if (config->extpthread_on_lcore>>i & 1) {
			pthread_affinity_set(i);
			INFO_PRINT("Tcp check time thread on %d lcore\n",i);
			break;
		}
	}

	INFO_PRINT("Tcp超时时间设置为：%d s\n",config->tcp_check_timeout);

	//用不退出的循环检测
	while (global_shared.continue_run) {

		//遍历所有的会话链接数，检测会话超时
		for (i=0; i<gsession_max_links && global_shared.continue_run; i++) {
			tcp_check_time(config, &gtcp_session_ring[i]);
		}

		//循环等待是为了程序退出时快
		for (i=0;i<config->tcp_check_timeout && global_shared.continue_run;i++) {
			sleep(1);
		}
	}

	INFO_PRINT("Tcp超时检测线程退出\n");
	return NULL;
}

/*
 *函数名称：tcp_check_time
 *函数功能：检测时间是否超时
 *输入参数：config             全局配置
 *          tcp_session_ring   会话的冲突链
 *输出参数：none
 *返回值：  none
*/
void tcp_check_time(struct yt_config *config, struct session_ring *tcp_session_ring)
{
	struct list_head *pos    = NULL;
	struct list_head *pos_tmp= NULL;

	pthread_spin_lock(&tcp_session_ring->spin_lock);
	{
		//遍历冲突链
		list_for_each_safe(pos,pos_tmp,&tcp_session_ring->session_head){

			struct tcp_session *session = (struct tcp_session*)pos;

			//时间未超时返回
			if (time(0) - session->time < config->tcp_check_timeout) {
				continue;
			}

			if (session->pkt_info) {
				data_transit_put(session->deal_record, session->pkt_info, session->data_buffer, session->buf_size, 
								 SESSION_STATUS_TIMEOUT);
			}
	
			tcp_session_free(session);
		}
	}
	pthread_spin_unlock(&tcp_session_ring->spin_lock);
}

