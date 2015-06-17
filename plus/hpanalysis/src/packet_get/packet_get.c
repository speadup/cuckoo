/***********************************************************
 *文件名：packet_get.c
 *创建人：Apologize
 *日  期：2014年04月10日
 *描  述：Get packet from device
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *
 ***********************************************************/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/time.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/mman.h>

#include <net/if.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h> 

#include <pthread.h>
#include <signal.h>	
//#include <execinfo.h>
#include <features.h> 
#include <ctype.h> 
#include "packet_get.h"
#include "packet_get_debug.h"

#include "net.h"
#include "config.h"
#include "pthread_affinity.h"
#include "stdyeetec.h"

//全局变量
int fd_main_sock;
char *rx_ring_buffer;
struct iovec *my_ring;
struct yt_config *config_info;

/*
 *函数名称: packet_get_real_start
 *函数功能: Get packet from socket;
 *输入参数: none
 *输出参数: none
 *返回值  : NULL
*/
static int packet_get_real_start(void);

/*
 *函数名称: packet_get_init
 *函数功能: 初始化模块,初始化计数器
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_init(void)
{
	INFO_PRINT("\n----->Packet get模块初始化..........\n");

	//计数器
	global_shared.retain_count ++;

	return YT_SUCCESSFUL;
}

/*
 *函数名称: packet_get_start
 *函数功能: 创建线程，开始抓包
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_start(void)
{
	INFO_PRINT("\nPacket get开启抓包线程\n");

	// Get configuration
	config_info = base_get_configuration();
	if (!config_info) {
		log_write(LOG_ERROR, "config get error - %s\n", __func__);
		return YT_FAILED;
	}

	// Set cpu affinity
	int i;
	for(i=0;i<YT_MAX_LCORE;i++){
		//判断并设定线程到指定cpu
		if(config_info->packetdeal_coremask>>i & 1){
			pthread_affinity_set(i);
			INFO_PRINT("rx_buffer_size=%d MB , rx_buffer_total_size=%ld MB, tp_block_size=%d byte,pre_packet_size=%d byte\ncpu_id=%d, interface_name=%s\n", config_info->rx_buffer_size, config_info->rx_buffer_total_size/1024/1024, config_info->tp_block_size, config_info->pre_packet_size, i, config_info->interface_name);
			break;
		}
	}

	//Catch packet
	return packet_get_real_start();
}


/*
 *函数名称: packet_get_real_start
 *函数功能: Get packet from socket;
 *输入参数: none
 *输出参数: none
 *返回值  : NULL
*/
static int packet_get_real_start(void)
{
	//创建socket
	fd_main_sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(fd_main_sock < 0){
		log_write(LOG_ERROR, "socket create error - %s\n", strerror(errno));
		return YT_FAILED;
	}

	//绑定地址
	if(!config_info->interface_name){
		log_write(LOG_ERROR, "interfac_name is NULL - %s\n", __func__);
		close(fd_main_sock);
		return YT_FAILED;
	}

	struct sockaddr_ll network_socket_addr;
	network_socket_addr.sll_family   = AF_PACKET;
	network_socket_addr.sll_protocol = htons(ETH_P_ALL);
	network_socket_addr.sll_ifindex  = if_nametoindex(config_info->interface_name); 
	if(bind(fd_main_sock, (struct sockaddr *)&network_socket_addr, sizeof(struct sockaddr_ll)) < 0){
		close(fd_main_sock);
		log_write(LOG_ERROR, "bind error - %s\n", strerror(errno));
		return YT_FAILED;
	}

	//设置socket选项
	struct tpacket_req req;
	req.tp_block_size = 12288; //3个4096
	req.tp_frame_size = 6144;
//	req.tp_block_nr = 1024*1024*12/req.tp_block_size;
	req.tp_block_nr = config_info->rx_buffer_total_size/req.tp_block_size;
//	req.tp_frame_nr = 1024*1024*12/req.tp_frame_size;
	req.tp_frame_nr = config_info->rx_buffer_total_size/req.tp_frame_size;

	if(setsockopt(fd_main_sock, SOL_PACKET, PACKET_RX_RING, (void *) &req, sizeof(req)) < 0){
		close(fd_main_sock);
		log_write(LOG_ERROR, "setsockopt error - %s\n", strerror(errno));
		DEBUG_PRINT("setsockopt error - %s\n", strerror(errno));
		return YT_FAILED;
	}

	//使用mmap进行抓包
	rx_ring_buffer = mmap(0, req.tp_block_size*req.tp_block_nr, PROT_READ|PROT_WRITE, MAP_SHARED, fd_main_sock, 0);
	if(rx_ring_buffer == MAP_FAILED){
		close(fd_main_sock);
		log_write(LOG_ERROR, "map error - %s\n", strerror(errno));
		DEBUG_PRINT("map error - %s\n", strerror(errno));
		return YT_FAILED;
	}

	int nIndex = 0,i = 0;
	struct pollfd pfd;
	struct tpacket_hdr *pHead = NULL;
	//循环从环形缓冲区中取包
	for(;global_shared.continue_run;) {

		pHead = (struct tpacket_hdr*)(rx_ring_buffer+ nIndex*req.tp_frame_size);;
		if(pHead->tp_status == TP_STATUS_USER){
			goto process_packet;
		}

		//Sleep when nothings happening
		pfd.fd=fd_main_sock;
		pfd.events=POLLIN;
		pfd.revents=0;
		if (poll(&pfd, 1, -1) < 0) {
			return YT_FAILED;
		}

process_packet:
        //尽力的去处理环形缓冲区中的数据frame，直到没有数据frame了
        for(i=0; i<req.tp_frame_nr; i++)
        {
            pHead = (struct tpacket_hdr*)(rx_ring_buffer+ nIndex*req.tp_frame_size);

            //由于frame都在一个环形缓冲区中，因此如果下一个frame中没有数据了，后面的frame也就没有frame了
            if(pHead->tp_status == TP_STATUS_KERNEL) {
                break;
			}

            //处理数据frame,切单包大小不能大于fram size
			if (req.tp_frame_size > pHead->tp_len) {
				net_data_deal((char*)pHead + pHead->tp_mac, pHead->tp_len);
			}

            //重新设置frame的状态为TP_STATUS_KERNEL
            pHead->tp_len = 0;
            pHead->tp_status = TP_STATUS_KERNEL;

            //更新环形缓冲区的索引，指向下一个frame
            nIndex++;
            nIndex%=req.tp_frame_nr;
        }
	}//end for()
	
	return YT_SUCCESSFUL;
}


/*
 *函数名称: packet_get_destory
 *函数功能: stop device and get packet
 *输入参数: none
 *输出参数: none
 *返回值  : Return 0(YT_SUCCESSFUL) if Success, otherwise -1(YT_FAILED) is returned.
*/
int packet_get_destroy()
{
	INFO_PRINT ("\n----->Packet get模块释放..........\n");
	if(0 <= fd_main_sock){
		close(fd_main_sock);
	}
	if(rx_ring_buffer){
		//记得要改，rx_buffer_total_size应该是传的参数
		munmap(rx_ring_buffer, config_info->rx_buffer_total_size);
	}
	if(my_ring){
		free(my_ring);
	}

	//计数器
	global_shared.retain_count --;

	return YT_SUCCESSFUL;
}

