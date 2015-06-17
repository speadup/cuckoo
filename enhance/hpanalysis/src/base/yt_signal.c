/***********************************************************
 *文件名：yt_signal.c
 *创建人：wangaichao
 *日  期：2013年09月28日
 *描  述：Deal signal
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include "stdyeetec.h"
#include "base.h"
#include "packet_get.h"
#include "net.h"
#include "data_transit.h"
#include "plugin_manager.h"
#include "feature_manager.h"
#include "storage_unit.h"
#include "public.h"
#include "mempool.h"

/*
 *函数名称: base_stop_program
 *函数功能: signal handler
 *输入参数: signum	Signal number
 *输出参数: none
 *返回值  : none
*/
static void base_stop_program(int signum)
{
	// set stop flag
	global_shared.continue_run = CONTINUE_EXIT;

	// run destroy func
	packet_get_destroy();
	net_destroy();
	data_transit_destroy();
    plugin_manager_destroy();
	feature_manager_destroy();
	storage_unit_destroy();
	base_destroy();
	public_destroy();
}

/*
 *函数名称: base_signal_set
 *函数功能: set signal handler
 *输入参数: void
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int base_signal_set(void)
{
	struct sigaction sigact;	

	memset(&sigact, 0 ,sizeof(struct sigaction));
	sigact.sa_handler = base_stop_program;

	if(sigaction(SIGINT, &sigact, NULL) == YT_FAILED) {
		log_write(LOG_ERROR, "%s\n", strerror(errno));
		return YT_FAILED;
	}

	signal(SIGRTMIN,mempool_display_info);

	return YT_SUCCESSFUL;
}

