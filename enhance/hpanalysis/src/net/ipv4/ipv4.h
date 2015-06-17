/***********************************************************
 *文件名：ipv4.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：解析ipv4数据的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#ifndef _IPV4_H_
#define _IPV4_H_

struct packet_info;

/*
 *函数名称：ipv4_init
 *函数功能：ipv4处理模块初始化
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int ipv4_init ();

/*
 *函数名称：ipv4_destroy
 *函数功能：ipv4处理模块资源释放
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void ipv4_destroy ();

/*
 *函数名称：ipv4_input
 *函数功能：处理ipv4数据的入口函数
 *输入参数：pkt_info           数据包信息
 *          ipv4_data          ipv4数据
 *          ipv4_data_size     数据大小
 *输出参数：none
 *返回值：  none
*/
void ipv4_input(struct packet_info *pkt_info, const char *ipv4_data,
		        unsigned ipv4_data_size);

#endif  //_IPV4_H_

