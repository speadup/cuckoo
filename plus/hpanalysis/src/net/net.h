/***********************************************************
 *文件名：net.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：net 模块的处理头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _NET_H_
#define _NET_H_

/*
 *函数名称：net_init
 *函数功能：完成net模块的初始化工作
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int net_init();

/*
 *函数名称：net_destroy
 *函数功能：释放net模块占用的资源
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void net_destroy();

/*
 *函数名称：net_data_deal
 *函数功能：通过net模块处理数据的函数
 *输入参数：net_data           net 数据
 *          net_data_size      数据的大小
 *输出参数：none
 *返回值：  none
*/
void net_data_deal(const char *net_data,const int net_data_size);

#endif   //_NET_H_

