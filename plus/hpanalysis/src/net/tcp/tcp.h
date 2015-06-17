/***********************************************************
 *文件名：tcp.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：tcp数据处理的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _TCP_H_
#define _TCP_H_

//对该结构体的声明
struct packet_info;

/*
 *函数名称：tcp_init
 *函数功能：完成tcp模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int tcp_init();

/*
 *函数名称：tcp_destroy
 *函数功能：释放tcp模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void tcp_destroy();

/*
 *函数名称：tcp_input
 *函数功能：处理tcp数据
 *输入参数：pkt_info           数据包信息
 *          tcp                tcp数据
 *          tcp_size           tcp数据的的大小
 *输出参数：none
 *返回值：  none
*/
void tcp_input(struct packet_info *pkt_info,const char *tcp,const unsigned tcp_size);


#endif  //_TCP_H_

