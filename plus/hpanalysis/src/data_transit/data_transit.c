/***********************************************************
 *文件名：data_transit.h
 *创建人：dreamsky_mo
 *日  期：2013年04月25日
 *描  述：network data transit to plugin_manager use ring
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "data_transit.h"
#include "data_transit_debug.h"
#include "stdyeetec.h"
#include "config.h"
#include "mempool.h"
//#include "tcp_session.h"
#include "net_defs.h"
#include "atomic.h"

#include <pthread.h>
#include <unistd.h>

#define TRANSIT_LIST_MIN 1

static struct list_head gtransit_list_head;

static pthread_spinlock_t spin_lock,*gspin_lock_transit = &spin_lock;
static atomic_t atomic_counter,*gtransit_node_counter = &atomic_counter;

/*
 *函数名称：data_transit_init
 *函数功能：data transit init function
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int data_transit_init(void)
{
	INFO_PRINT("\n----->data_transit模块初始化..........\n");

	INIT_LIST_HEAD (&gtransit_list_head);

	pthread_spin_init(gspin_lock_transit,0);
	atomic_set(gtransit_node_counter, 0);

	//计数器
	global_shared.retain_count++;
	
	struct yt_config *config = base_get_configuration();
	assert (config);

	
	return YT_SUCCESSFUL;
}

/*
 *函数名称：data_transit_destroy
 *函数功能：data transit destroy
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void data_transit_destroy(void)
{
	INFO_PRINT("\n----->data_transit模块释放..........\n");
	
	//计数器
	global_shared.retain_count--;
	

	pthread_spin_destroy(gspin_lock_transit);
}

/*
 *函数名称：data_transit_get
 *函数功能：plugin_manager get transit data
 *输入参数：get_deal_data      获取处理数据的链表头 
 *输出参数：deal_record        返回deal_record
 *返回值：  none
*/
int data_transit_get(struct list_head *get_deal_data,struct session_deal_record **deal_record)
{
	int ret_value = YT_SUCCESSFUL;
	int get_max_cache = base_get_configuration()->deal_thread_max_cach;
	struct list_head *list_site_tmp = NULL;
	struct list_head *transit = NULL;
	struct data_transit *tmp = NULL;
	struct session_deal_record *record_tmp = NULL;

	pthread_spin_lock(gspin_lock_transit);

	if (UNLIKELY(list_empty(&gtransit_list_head))) {
		pthread_spin_unlock(gspin_lock_transit);
		return YT_FAILED;
	}

	list_site_tmp = gtransit_list_head.next;
		
	//找一个可以处理的会话
	while(((struct data_transit*)(list_site_tmp))->deal_record && 
			SESSION_NOT_DEAL == ((struct data_transit*)(list_site_tmp))->deal_record->session_deal_flag){
			
		list_site_tmp = list_site_tmp->next;
		
		if (UNLIKELY(list_site_tmp == &gtransit_list_head)) {
			pthread_spin_unlock(gspin_lock_transit);
			usleep (100);

			return YT_FAILED;
		}
	}
		
	do {
		transit = list_site_tmp;
		
		//先处理该会话上次处理记录的位置包
		if((((struct data_transit*)transit)->deal_record)->session_index){
			
			//跳到上次处理位置
			transit  = (((struct data_transit*)transit)->deal_record)->session_index;
			list_site_tmp = transit->next;
			if(list_site_tmp == &gtransit_list_head){
				pthread_spin_unlock(gspin_lock_transit);
				usleep (100);

				return YT_FAILED;
			}
			else{
				//会话可以删除了，拿掉最后一个包，删除record，直接返回
				if (((struct data_transit*)transit)->deal_record->record_destory_flag == RECORD_CAN_DESTORY){
//atomic_dec(gtransit_node_counter);
//DEBUG_PRINT("------transit_node_counter:%d\n",atomic_read(gtransit_node_counter));
					
					list_del(transit);
					
					mempool_free(((struct data_transit*)transit)->deal_record);
					mempool_free(((struct data_transit*)transit)->l4_data);
					mempool_free((struct data_transit*)transit);
					
					pthread_spin_unlock(gspin_lock_transit);

					return YT_FAILED;
				}
				
				//拿掉包，判断下个包是否是该会话
				(((struct data_transit*)transit)->deal_record)->session_index = NULL;
				
				record_tmp = ((struct data_transit*)(transit))->deal_record;
				
				list_del(transit);

//atomic_dec(gtransit_node_counter);
//DEBUG_PRINT("------transit_node_counter:%d\n",atomic_read(gtransit_node_counter));

				mempool_free(((struct data_transit*)transit)->l4_data);
				mempool_free((struct data_transit*)transit);

				//如果下个会话不是一个会话了，那么返回，只是把记录位置的包删掉
				if(record_tmp != ((struct data_transit*)(list_site_tmp))->deal_record) {
					pthread_spin_unlock(gspin_lock_transit);
					
					return YT_FAILED;
				}
				else{
					//同一个会话，开始取包
					transit = list_site_tmp;
				}
			
			}
		}
		
		list_add_tail(&((struct data_transit*)transit)->deal_node,get_deal_data);
	
		list_site_tmp = list_site_tmp->next;
		
		//保证链表中有一个数据
		if (LIKELY(list_site_tmp == &gtransit_list_head)) {
			goto ret_trasit_get;
		}

		//判断下一个节点数据是否为该链接的数据record地址不同的
		if(((struct data_transit*)transit)->deal_record != ((struct data_transit*)(list_site_tmp))->deal_record) {
			goto ret_trasit_get;
		}

		get_max_cache --;

	}while (LIKELY(get_max_cache && global_shared.continue_run));

ret_trasit_get:

	list_for_each_entry(tmp, get_deal_data, deal_node){
		if((tmp->deal_node).next != get_deal_data){
			//不是最后一个，全部从链表删掉
			list_del(&(tmp->data_transit_node));
	
//atomic_dec(gtransit_node_counter);
//DEBUG_PRINT("------transit_node_counter:%d\n",atomic_read(gtransit_node_counter));
		}
		else{
			//最后一个不del，记录处理位置
			tmp->deal_record->session_index = &(tmp->data_transit_node);
		}
	}	

	//设置会话正在处理的标志
    ((struct data_transit*)(transit))->deal_record->session_deal_flag = SESSION_NOT_DEAL;
	*deal_record = ((struct data_transit*)(transit))->deal_record;

	pthread_spin_unlock(gspin_lock_transit);

	return ret_value;
}

/*
 *函数名称：data_transit_free
 *函数功能：plugin_manager free transit data
 *输入参数：free_data1     释放已经从主链表无关联的内存
 *          free_data2     释放与主链表有关联的内存
 *          deal_record    会话的record
 *返回值：  none
*/
void data_transit_free(struct list_head *free_data1,struct session_deal_record *deal_record)
{
	struct data_transit *pos = NULL;
	struct data_transit *tmp = NULL;

	list_for_each_entry_safe (pos,tmp,free_data1,deal_node) {
		if((pos->deal_node).next != free_data1){
			//不是最后一个的，全部删除
			list_del(&(pos->deal_node));

			mempool_free (pos->l4_data);
			mempool_free (pos);
//atomic_dec(gtransit_node_counter);
//DEBUG_PRINT("------transit_node_counter:%d\n",atomic_read(gtransit_node_counter));
			
		}
		else{
			//最后一个只从链表中删掉，其他内存由下次删
			list_del(&(pos->deal_node));
		}
	}

	if(deal_record){
		deal_record->session_deal_flag = SESSION_CAN_DEAL;
	}
}

/*
 *函数名称：data_transit_put
 *函数功能：net module put data 
 *输入参数：deal_record        记录插件处理的信息
 *		    pkt_info           数据包信息包括mac对，ip对等。
 *          l4_data            四层要解析的数据
 *          l4_data_size       四层要解析数据的大小
 *          status             会话状态
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int data_transit_put(struct session_deal_record *deal_record, struct packet_info *pkt_info, 
		             char *l4_data, int l4_data_size, unsigned char status)
{
//	mempool_free(pkt_info);
//	mempool_free(l4_data);
//	return YT_SUCCESSFUL;
	
	assert(deal_record && pkt_info);
/*	if (SESSION_STATUS_CLOSE == deal_record->session_status || 
		SESSION_STATUS_TIMEOUT == deal_record->session_status) {
		return YT_SUCCESSFUL;
	}
*/
	struct data_transit *transit = mempool_malloc(sizeof(struct data_transit));
	if (UNLIKELY(!transit))
	{
		log_write(LOG_ERROR, "mempool malloc %d size error,%s:%d\n",sizeof(struct data_transit),__FILE__,__LINE__);
		return YT_FAILED;
	}
	
	transit->deal_record   = deal_record;
	transit->pkt_info      = *pkt_info;
	transit->l4_data       = l4_data;
	transit->l4_data_size  = l4_data_size;
	transit->session_status= status;

	deal_record->session_status = status;
	assert(transit->deal_record);

	list_add_tail(&transit->data_transit_node, &gtransit_list_head);

//atomic_inc(gtransit_node_counter);
//DEBUG_PRINT("------transit_node_counter:%d\n",atomic_read(gtransit_node_counter));
	return YT_SUCCESSFUL;
}

