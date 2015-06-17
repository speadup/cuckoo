/***********************************************************
 *文件名：tcp_check_time.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：tcp超时检测的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _TCP_CHECK_TIME_
#define _TCP_CHECK_TIME_

#include <pthread.h>
#include "list.h"
#include "tcp_session.h"
#include "atomic.h"

//tcp会话节点
struct session_ring
{
	struct list_head session_head;  //tcp会话的头，使用拉链法解决hash冲突
	pthread_spinlock_t spin_lock;   //该节点中，会话锁（冲突时可能有多个会话）
	atomic_t hash_max_num;			//节点最大hash数
};

/*
 *函数名称：tcp_check_time_init
 *函数功能：初始化tcp超时检测模块
 *输入参数：tcp_session_ring   会话链表
 *          session_max_links  会话的最大链接数
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int tcp_check_time_init(struct session_ring *tcp_session_ring,unsigned session_max_links);

/*
 *函数名称：tcp_check_time_destroy
 *函数功能：释放函数
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_check_time_destroy();

/*
 *函数名称：tcp_session_free
 *函数功能：释放tcp会话
 *说    明：该函数的实现在 tcp_session.c 文件中
 *输入参数：session            tcp session
 *输出参数：none
 *返回值：  none
*/
void tcp_session_free(struct tcp_session *session);

#endif  //_TCP_CHECK_TIME_

