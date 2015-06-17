/***********************************************************
 *文件名：public.h
 *创建人：小菜_默
 *日  期：2013年10月25日
 *描  述：public interface
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _PUBLIC_H_
#define _PUBLIC_H_

/*
 *函数名称：public_init
 *函数功能：public module init
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int public_init();

/*
 *函数名称：public_destroy
 *函数功能：public module destroy
 *输入参数：none
 *输出参数：none
 *返回值：  none 
*/
void public_destroy();

#endif  //__PUBLIC_H_

