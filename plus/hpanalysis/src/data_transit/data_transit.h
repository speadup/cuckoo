/***********************************************************
 *文件名：data_transit.h
 *创建人：dreamsky_mo
 *日  期：2014年04月25日
 *描  述：network data transit to plugin_manager
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _DATA_TRANSIT_H_
#define _DATA_TRANSIT_H_

#include "net_defs.h"
#include "list.h"

//表示会话的状态
enum {
	SESSION_STATUS_RECV,            //会话正常接收数据
	SESSION_STATUS_CLOSE,           //会话已经关闭
	SESSION_STATUS_TIMEOUT,         //会话超时
};

struct session_deal_record;                     //对该结构进行声明

struct data_transit {
	struct list_head data_transit_node;         //数据中转模块节点
	struct list_head deal_node;                 //数据处理节点

	unsigned char del_node_flag;                //是否从数据中转删除标志 0->没有删除  1->已经删除
#define TRANSIT_NOT_DEL_NODE 0
#define TRANSIT_DEL_NODE     1

	unsigned char deal_node_flag;
#define TRANSIT_NOT_DEAL     0                  //该数据没有被处理
#define TRANSIT_ALREADY_DEAL 1                  //该数据已经被处理
	
	struct session_deal_record *deal_record;    //记录了插件处理的内容
	struct packet_info pkt_info;                //数据包信息
	char *l4_data;                              //4层要处理的数据
	int  l4_data_size;                          //4层处理数据的大小
	unsigned char session_status;	       	    //会话状态 使用SESSION_STATUS_*
};

/*
 *函数名称：data_transit_init
 *函数功能：data transit init function
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int  data_transit_init();

/*
 *函数名称：data_transit_destroy
 *函数功能：data transit destroy
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
void data_transit_destroy();

/*
 *函数名称：data_transit_get
 *函数功能：plugin_manager get transit data
 *输入参数：get_deal_data      获取处理数据的链表头 
 *输出参数：deal_record        返回deal_record
 *返回值：  none
*/
int data_transit_get(struct list_head *get_deal_data,struct session_deal_record **deal_record);

/*
 *函数名称：data_transit_free
 *函数功能：plugin_manager free transit data
 *输入参数：free_data1     释放已经从主链表无关联的内存
 *          free_data2     释放与主链表有关联的内存
 *          deal_record    会话的record
 *返回值：  none
*/
void data_transit_free(struct list_head *free_data1, struct session_deal_record *deal_record);

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
		             char *l4_data, int l4_data_size, unsigned char status);

#endif  //_DATA_TRANSIT_H_

