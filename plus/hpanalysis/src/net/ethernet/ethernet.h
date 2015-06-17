/***********************************************************
 *文件名：ethernet.h
 *创建人：小菜_默
 *日  期：2013年11月13日
 *描  述：二层数据处理对外的头文件
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _ETHERNET_H_
#define _ETHERNET_H_

/*
 *函数名称：ethernet_input
 *函数功能：接收二层的数据包进行处理
 *输入参数：eth_data           二层数据
 *          eth_data_size      数据大小
 *输出参数：none
 *返回值：  none
*/
void ethernet_input(const char *eth_data,const unsigned eth_data_size);

#endif  //_ETHERNET_H_

