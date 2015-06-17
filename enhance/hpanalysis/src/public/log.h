/***********************************************************
 *文件名：log.h
 *创建人：小菜_默
 *日  期：2013年10月15日
 *描  述：record log
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#ifndef _LOG_H_
#define _LOG_H_

#include <syslog.h>

//log level
#define LOG_ERROR   3
#define LOG_WARNING 4
#define LOG_INFO    6

/*
 *函数名称：log_init
 *函数功能：init log
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void log_init();

/*
 *函数名称：log_write
 *函数功能：write log
 *输入参数：priority     log level
 *          format
 *输出参数：none
 *返回值：  none 
*/
//void log_write(int priority, const char *format, ...);
#define log_write(priority,fmt,args...) syslog(priority,fmt,##args);

/*
 *函数名称：log_destroy
 *函数功能：destroy log
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void log_destroy();

#endif  //_LOG_H_

