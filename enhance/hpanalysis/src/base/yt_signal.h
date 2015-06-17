/***********************************************************
 *文件名：yt_signal.h
 *创建人：wangaichao
 *日  期：2013年09月28日
 *描  述：Deal signal
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *
 ***********************************************************/
#ifndef _SIGNAL_H_
#define _SIGNAL_H_

/*
 *函数名称: base_signal_set
 *函数功能: Set signal handler
 *输入参数: void
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int base_signal_set(void);

#endif	//yt_signal.h
