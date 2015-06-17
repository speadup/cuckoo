/***********************************************************
*文件名：plugin_proxy_socks4.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：socks4插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_proxy_socks4.h"
#define PROTOCOLID "0071"
#define PROTOCOLSUBID "007100000001"
#define PROTOCOLNAME  "PROXY_SOCKS4"

//文件存放路径
static char *socks4_plugin_path = NULL;

//表名的时间后缀
static char socks4_suffix[COMMON_LENGTH_32] = {0};

//同花顺端口数组
static int SOCKS4_PORT[1] = {1080};

struct socks4_header
{
    unsigned char version;
    unsigned char command;
    unsigned short port;
    unsigned int ip;
    unsigned char data[0];  /* here is username */
};

/*
*函数名称：plugin_init
*函数功能：初始化socks4插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_socks4 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "socks4 base_get_configuration() error\n");
		return YT_FAILED;
	}

	socks4_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = PROXY_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < PROXY_PORT_NUM; i++){
		ppi.port = SOCKS4_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "socks4 plugin_manager_register() error\n");
			return YT_FAILED;
		}
	}

	return YT_SUCCESSFUL;
}

/*
函数名称：plugin_deal
函数功能：处理数据包
输入参数：dtts：待处理的数据结构体
		  thread_num：处理的线程号，用于存储模块
*输出参数：无
*返回值：0成功 -1失败
*/

static int plugin_deal(const struct data_transit *dtts, const int thread_num)
{

	if(!dtts){
		return YT_FAILED;
	}

	struct socks4_record *socks4_rec = NULL;

	socks4_rec = (struct socks4_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(socks4_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是bbs处理后的后续杂包
    if(socks4_rec && socks4_rec->plugin_ds.plugin_type == PLUGIN_TYPE_BBS) {
        return YT_FAILED;
    }

	//socks4客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		if(plugin_deal_client(dtts, thread_num) == YT_FAILED) {
			return YT_FAILED;
		}

		return YT_SUCCESSFUL;
	}

	return YT_FAILED;
}

/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*输入参数：dtts:提供待处理的数据
*输出参数：socks4_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num)
{
	time_t socks4_time;
	struct tm *socks4_time_now;

    struct socks4_header *p_socks4 = (struct socks4_header*)dtts->l4_data;
    struct socks4_record *socks4_rec = (struct socks4_record*)((dtts->deal_record)->record);

    if (dtts->l4_data_size >= 8
        && dtts->l4_data_size <= 128
        && p_socks4->version == 0x04
        && p_socks4->command == 0x01) {
        /* establish a TCP/IP stream connection */
        if (!socks4_rec) {
	        socks4_rec = (struct socks4_record*)mempool_malloc(sizeof(struct socks4_record));
	        if(!socks4_rec){
	        	log_write(LOG_ERROR, "socks4_rec malloc() error\n");
	        	return YT_FAILED;
	        }
	        dtts->deal_record->record = socks4_rec;
        }

        if (p_socks4->data[0]) {
            strncpy(socks4_rec->account, p_socks4->data, ACCOUNT_LENGTH);
        }
        socks4_rec->action = 1;
    } else {
		return YT_FAILED;
    }


	//先获得下时间
	time(&socks4_time);
	socks4_time_now = localtime(&socks4_time);	
	sprintf(socks4_rec->date, "%04d-%02d-%02d", socks4_time_now->tm_year + 1900, 
			socks4_time_now->tm_mon + 1, socks4_time_now->tm_mday);

	sprintf(socks4_suffix, "%04d%02d%02d", socks4_time_now->tm_year + 1900,
			socks4_time_now->tm_mon + 1, socks4_time_now->tm_mday);

	sprintf(socks4_rec->time, "%02d:%02d:%02d", socks4_time_now->tm_hour, 
			socks4_time_now->tm_min, socks4_time_now->tm_sec);

	plugin_data_storage(dtts, socks4_rec, thread_num);


	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 socks4_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct socks4_record *socks4_rec, const int thread_num)
{
	if(!dtts || !socks4_rec){
		return YT_FAILED;
	}

	struct plugin_element element_tmp;

	memset(&element_tmp, 0, sizeof(struct plugin_element));

	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);
	//	vlan_id
	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	//table name
	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", PROXY_PRIFIX, socks4_suffix);

	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}


	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%d','%s %s','%s','%s','%d','%s','%s','%d','%s','%s','%s', '%s');",
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,faction,fregtime,fsourceip,fsourcemac,fsourceport,fdestinationip,fdestinationmac,fdestinationport,faccount,fpassword,fitem1,fitem2",
			PLACECODE,
			PLACENAME,
			PROTOCOLID,
			PROTOCOLSUBID,
			PROTOCOLNAME,
			socks4_rec->action,
			socks4_rec->date,
			socks4_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			socks4_rec->account,
			socks4_rec->password,
			socks4_rec->device_type,
			socks4_rec->os_ver
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "socks4 storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：socks4_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct socks4_record *socks4_dst)
{	
	if(socks4_dst){
		mempool_free(socks4_dst);
	}
}
