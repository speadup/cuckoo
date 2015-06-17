/***********************************************************
 *文件名：config.h
 *创建人：wangaichao
 *日  期：2013年09月28日
 *描  述：Deal configuration info
 *
 *修改人：sunjinpeng
 *日  期：2014年04月10日
 *描  述：修改struct yt_config结构体
 *
 ***********************************************************/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "stdyeetec.h"

struct yt_config {

	char *prog_name;             //-g 程序名称

	//packet_get module
	unsigned long long packetdeal_coremask;	// -c packet deal cpu mask
	short rx_buffer_size;        // -s 环形缓冲区的每个单元大小
	long rx_buffer_total_size;   // -b 环形缓冲区的总大小
	char *interface_name;        // -i 网卡接口名称
	int tp_block_size;           // -t 连续块的最小大小
	int pre_packet_size;         // -p 每个包的最大大小

    //storage_unit module
    short storage_w2f_timeout;      // -w 存储模块写文件超时控制

	// net module
    short tcp_check_timeout;        //-o tcp会话超时时间

	// data_transit module
	unsigned number_of_links;		// -l the max links

	// plugin_manager module
	char *plugin_dir_path;			// -d plugin directory path
	unsigned long long datadeal_coremask;	// -C data deal cpu mask

	// feature_manager module 
	char *fm_db_name;				// -D database file dir

	// plugin_work module
	char *plugin_storage_path;		// -S plugin store file path

	// public module
    unsigned long long extpthread_on_lcore;  // -T which lcore to bind extra thread like timer or others,修改为字符串，统一参数格式 
	
	// plugin_manager
	unsigned deal_thread_max_cach;

	//net
	unsigned hash_max_list;			//hash max list

	unsigned prog_max_use_mem;      //-P 程序最大使用的内存数
};

/*
 *函数名称: base_get_configuration
 *函数功能: Get configuration info address
 *输入参数: void
 *输出参数: none
 *返回值  : The address of config info
*/
struct yt_config* base_get_configuration(void);

/*
 *函数名称: base_parse_arg
 *函数功能: Parse arguments
 *输入参数: argc	Arguments num
 			argv	Arguments address
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int base_parse_arg(int argc, char *argv[]);

//检测到的网卡的个数。需要根据端口个数来控制线程个数。
extern int nb_ports;

#endif //config.h
