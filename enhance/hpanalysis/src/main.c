#include <stdio.h>
#include <unistd.h>

#include "public.h"
#include "stdyeetec.h"
#include "log.h"
#include "base.h"
#include "config.h"
#include "storage_unit.h"
#include "feature_manager.h"
#include "plugin_manager.h"
#include "data_transit.h"
#include "net.h"
#include "packet_get.h"

#define SLEEP_TIME	10	// 10 senconds

/*
 *函数名称: main
 *函数功能: run program
 *输入参数: argc	Arguments num
			argv	Arguments address
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int main(int argc, char *argv[])
{
	int ret = 0;

	// arg and signal deal
	ret = base_parse_arg(argc, argv);
	if (ret != YT_SUCCESSFUL) {
		fprintf(stderr, "base_parse_arg() error\n");
		return YT_FAILED;
	}

	// set run flag
	global_shared.continue_run = 1;

	// init
	ret = public_init();
	if (ret != YT_SUCCESSFUL) {
		fprintf(stderr, "public_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = base_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "base_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = packet_get_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "packet_get_init() error, program exit\n");
		return YT_FAILED;
	}
    
	ret = net_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "net_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = data_transit_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "data_transit_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = feature_manager_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "feature_manager_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = plugin_manager_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "plugin_manager_init() error, program exit\n");
		return YT_FAILED;
	}

	ret = storage_unit_init();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "storage_unit_init() error, program exit\n");
		return YT_FAILED;
	}

	// start
	ret = packet_get_start();
	if (ret != YT_SUCCESSFUL) {
		log_write(LOG_ERROR, "packet_get_start() error, program exit\n");
		return YT_FAILED;
	}

	log_write(LOG_ERROR, "program init success\n");

	while (global_shared.retain_count) {
		usleep (100);
	}

	return YT_SUCCESSFUL;
}
