/***********************************************************
 *文件名：udp.h
 *创建人：小菜_默
 *日  期：2014年08月12日
 *描  述：udp数据处理的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/
#ifndef _UDP_H_
#define _UDP_H_

//对该结构体的声明
struct packet_info;

/*
 *函数名称：udp_init
 *函数功能：完成udp模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int udp_init();

/*
 *函数名称：udp_destroy
 *函数功能：释放udp模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void udp_destroy();

/*
 *函数名称：udp_input
 *函数功能：处理udp数据
 *输入参数：pkt_info           数据包信息
 *          udp                udp数据
 *          udp_size           udp数据的的大小
 *输出参数：none
 *返回值：  none
*/
void udp_input(struct packet_info *pkt_info,const char *udp,const unsigned udp_size);

#endif  //_UDP_H_

