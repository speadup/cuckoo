/***********************************************************
 *文件名：packet_get.h
 *创建人：Apologize
 *日  期：2014年04月10日
 *描  述：Get packet from device
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *
 ***********************************************************/

#ifndef __PACKET_GET_H__
#define __PACKET_GET_H__

/*
 *函数名称: packet_get_init
 *函数功能: init device 
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_init(void);

/*
 *函数名称: packet_get_start
 *函数功能: start device and get packet
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_start(void);

/*
 *函数名称: packet_get_destory
 *函数功能: stop device and get packet
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_destroy(void);

#endif // packet_get.h
