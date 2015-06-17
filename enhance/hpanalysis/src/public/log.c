/***********************************************************
 *文件名：log.c
 *创建人：小菜_默
 *日  期：2013年10月15日
 *描  述：record log process file .c
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "log.h"
#include "stdyeetec.h"
#include "config.h"

void log_init()
{
	INFO_PRINT("Public->Log模块初始化\n");

	struct yt_config *config = base_get_configuration();
	assert (config);

	openlog(config->prog_name, LOG_PID|LOG_CONS, LOG_USER);
}

void log_destroy()
{
	INFO_PRINT("Public->Log模块释放\n");
	closelog();
}
/*
void log_write(int priority, const char *format, ...)
{
	syslog(priority,format,);
}
*/

