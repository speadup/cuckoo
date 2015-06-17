/***********************************************************
 *文件名：hash.h
 *创建人：小菜_默
 *日  期：2013年11月23日
 *描  述：该文件是从libnids中cpy的，修改了函数的定义，由
 *        单纯的ipv4添加了ipv6
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#ifndef _HASH_H_
#define _HASH_H_

/*
 *函数名称：init_hash
 *函数功能：初始化hash
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void init_hash();

/*
 *函数名称：mkhash
 *函数功能：根据输入的ip和port返回hash值
 *输入参数：ip_src             ipv4 source ip
 *          ip6_src            ipv6 source ip
 *          sport              source port
 *          ip_dest            ipv4 dest ip
 *          ip6_dest           ipv6 dest ip
 *          dport              dest port
 *输出参数：none
 *返回值：  hash value
*/
unsigned mkhash (unsigned ip_src,unsigned *ip6_src,unsigned short sport,
		         unsigned ip_dest,unsigned *ip6_dest,unsigned short dport);

#endif //_HASH_H_

