/***********************************************************
 *文件名：config.c
 *创建人：wangaichao
 *日  期：2013年09月28日
 *描  述：Deal configuration info
 *
 *修改人：sunjinpeng
 *日  期：2014年01月10日
 *描  述：修改参数解析接口
 *
 ***********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "log.h"

// init config info
static struct yt_config config_info = {
	.number_of_links = 1024,					// -l the max links
    .tcp_check_timeout = 20,					// -o tcp超时检查时间
    .storage_w2f_timeout = 20,					// -w 写文件超时时间
	.rx_buffer_size = 1,						// -s 环形缓冲的每个单元大小(单位：MB) 
	.rx_buffer_total_size = 1024*1024*10,		// -b 环形缓冲区的总大小
	.tp_block_size = 4096,						// -t 连续块的最小大小
	.pre_packet_size = 2048,					// -p 每个包的最大大小
	.packetdeal_coremask = 0,					// 赋值为0是为了下边参数解析结束后做判断用
	.datadeal_coremask = 0,      
	.extpthread_on_lcore = 0,   
	.deal_thread_max_cach = 10,					//处理线程最大缓存包数
	.hash_max_list = 10,						//hash链表最大数
	.prog_max_use_mem=30*1024*1024              //程序最大使用的内存数
};

/*
 *函数名称: usage
 *函数功能: show usage message for user
 *输入参数: void
 *输出参数: none
 *返回值  : void
*/
void usage(void)
{
	printf("\n  Usage: hpanalysis [OPTION]...        \n");
	printf("    Catch packet and analysis            \n");
	printf("-----must pass parameters----------------\n");
	printf("    -T bind pthread on which lcore       \n");
	printf("    -g programe name                     \n");
	printf("    -c packet get cpu mask               \n");
	printf("    -i network card interface name       \n");
	printf("    -d plugin directory path             \n");
	printf("    -C data deal cpu mask                \n");
	printf("    -D database file dir                 \n");
	printf("    -S plugin storage path               \n");
	printf("-----optional parameters-----------------\n");
	printf("    -s circular buffer size of each unit \n");
	printf("    -o tcp check timeout                 \n");
	printf("    -b total size of the ring buffer     \n");
	printf("    -w storage write to file timeout     \n");
	printf("    -l the max links                     \n");
	printf("    -t minimum size continuous block     \n");
	printf("    -p maximum size of each package      \n");
	printf("    -m max cach packet of deal thread    \n");
	printf("	-h hash max list				     \n");
	printf("	-P program max use memory(MB)	     \n");

}

/*
 *函数名称: base_get_configuration
 *函数功能: Get configuration info address
 *输入参数: void
 *输出参数: none
 *返回值  : The address of config info
*/
struct yt_config* base_get_configuration(void)
{
	return &config_info;
}

/*
 *函数名称: base_parse_arg
 *函数功能: Parse arguments
 *输入参数: argc	Arguments num
 			argv	Arguments address
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int base_parse_arg(int argc, char *argv[])
{
	//int packet_deal_dev_num = 0;	// 为了保证网口参数不超过YT_MAX_DEV
	//int data_deal_dev_num = 0;		// 为了保证-c -C参数可以成对出现

	int opt;

	if (argc <= 1) {
		usage();
		return YT_FAILED;
	}

	/* 在写参数的时候需要注意-c 和 -C 这两个参数
	 * -c 代表数据包获取的cpu_mask
	 * -C 代表数据处理的cpu_mask
	 * 两者必须成对出现，如-c EEEE -C FFFF
	 * 一对-c -C就代表一个网口的数据包获取和数据处理的cpu_mask
	 * 若有多个网卡需要处理，则写多对即可,如 -c EEEE -C FFFF -c EEEE -C FFFF
	 */
	while((opt = getopt(argc, argv, "b:c:C:d:l:D:g:i:p:s:S:t:T:o:w:m:h:P:")) != YT_FAILED) {
		switch (opt) {
			case 'b':
				config_info.rx_buffer_total_size = atoi(optarg);
				break;
			case 'c':	
				config_info.packetdeal_coremask = strtoull(optarg, NULL, 16);
				break;
			case 'C':	
				config_info.datadeal_coremask = strtoull(optarg, NULL, 16);
				break;
			case 'd':
				config_info.plugin_dir_path = optarg;
				break;
			case 'l':
				config_info.number_of_links = atoi(optarg);
				break;
			case 'D':
				config_info.fm_db_name = optarg;
				break;
			case 'g':
				config_info.prog_name = optarg;
			case 'i':
				config_info.interface_name = optarg;
				break;
			case 'p':
				config_info.pre_packet_size = atoi(optarg);
				break;
			case 's':
				config_info.rx_buffer_size = atoi(optarg);
				break;
			case 'S':
				config_info.plugin_storage_path = optarg;
				break;
			case 't':
				config_info.tp_block_size = atoi(optarg);
				break;
			case 'T':
				config_info.extpthread_on_lcore = strtoull(optarg, NULL, 16);
				break;
			case 'o':
				config_info.tcp_check_timeout = atoi(optarg);
				break;
			case 'w':
				config_info.storage_w2f_timeout = atoi(optarg);
				break;
			case 'm':
				config_info.deal_thread_max_cach = atoi(optarg);
				break;
			case 'h':
				config_info.hash_max_list = atoi(optarg);
				break;
			case 'P':
				config_info.prog_max_use_mem = atoi(optarg)*1024*1024; 
				break;
			default:
				usage();
				return YT_FAILED;
				break;
		}	
	}

	// 简单参数检查
	if (config_info.plugin_dir_path == NULL){
		fprintf(stderr, "lack of parameter:-d plugin directory path\n");
		goto false;
	}
	if(config_info.fm_db_name == NULL){
		fprintf(stderr, "lack of parameter:-D database file dir\n");
		goto false;
	}
	if(config_info.plugin_storage_path == NULL){
		fprintf(stderr, "lack of parameter:-S plugin storage path\n");
		goto false;
	}
	if(config_info.interface_name == NULL){
		fprintf(stderr, "lack of parameter:-i network card interface name\n");
		goto false;
	}
	if(config_info.datadeal_coremask == 0){
		fprintf(stderr, "lack of parameter:-C data deal cpu mask\n");
		goto false;
	}
	if(config_info.extpthread_on_lcore == 0){
		fprintf(stderr, "lack of parameter:-T bind pthread on which lcore\n");
		goto false;
	}
	if(config_info.packetdeal_coremask == 0) {
		fprintf(stderr, "lack of parameter:-c packet get cpu mask\n");
		goto false;
	}

	return YT_SUCCESSFUL;

false:
	return YT_FAILED;
}
