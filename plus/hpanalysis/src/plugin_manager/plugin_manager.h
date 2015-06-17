#ifndef _PLUGIN_MANAGER_H_
#define _PLUGIN_MANAGER_H_

#include "data_transit.h"
#include <inttypes.h>

extern int plugin_manager_thread_num;

// plugin info struct
struct plugin_info {
	uint32_t plugin_id;		// plugin id
	uint32_t port;			// watch port
	int (*func_plugin_deal)(const struct data_transit *, const int thread_num);	// plugin deal func
	struct plugin_info * next;	// next point
};

/*
 *函数名称: plugin_manager_init
 *函数功能: load plugins and start smp deal
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_init(void);

/*
 *函数名称: plugin_manager_destroy
 *函数功能: stop plugin manager deal
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_destroy(void);

/*
 *函数名称: plugin_manager_register
 *函数功能: register plugin
 *输入参数: ppi		plugin info point
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_register(struct plugin_info * ppi);

#endif // plugin_manager.h
