/***********************************************************
*文件名：plugin_tonghuashun.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：tonghuashun插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_stock_tonghuashun.h"
#define PROTOCOLID "9005"
#define PROTOCOLSUBID "900500000003"
#define PROTOCOLNAME  "同花顺-股票"

//文件存放路径
static char *tonghuashun_plugin_path = NULL;

//表名的时间后缀
static char tonghuashun_suffix[COMMON_LENGTH_32] = {0};

//同花顺端口数组
static int STOCK_PORT[1] = {80};

/*
*函数名称：plugin_init
*函数功能：初始化tonghuashun插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_tonghuashun 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "tonghuashun base_get_configuration() error\n");
		return YT_FAILED;
	}

	tonghuashun_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = STOCK_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < STOCK_PORT_NUM; i++){
		ppi.port = STOCK_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "tonghuashun plugin_manager_register() error\n");
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

	struct tonghuashun_record *tonghuashun_rec = NULL;

	tonghuashun_rec = (struct tonghuashun_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(tonghuashun_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是bbs处理后的后续杂包
    if(tonghuashun_rec && tonghuashun_rec->plugin_ds.plugin_type == PLUGIN_TYPE_BBS) {
        return YT_FAILED;
    }

	//tonghuashun客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
        if (!tonghuashun_rec) {
		    tonghuashun_rec = (struct tonghuashun_record*)mempool_malloc(sizeof(struct tonghuashun_record));
		    if(!tonghuashun_rec){
		    	log_write(LOG_ERROR, "tonghuashun_rec malloc() error\n");
		    	return YT_FAILED;
		    }
		    dtts->deal_record->record = tonghuashun_rec;
        }

		if(plugin_deal_client(dtts, tonghuashun_rec, thread_num) == YT_FAILED) {
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
*输出参数：tonghuashun_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct tonghuashun_record *tonghuashun_rec, const int thread_num)
{
	if(!tonghuashun_rec){
		return YT_FAILED;
	}
	
	time_t tonghuashun_time;
	struct tm *tonghuashun_time_now;

	do {
        if (!memcmp("GET /verify?reqtype=",dtts->l4_data,4)
            && strstr(dtts->l4_data, "Host: authclient.10jqka.com.c")) {
            plugin_data_extract(dtts->l4_data, "account=", "&", tonghuashun_rec->account, ACCOUNT_LENGTH);
            plugin_data_extract(dtts->l4_data, "passwd=", "&", tonghuashun_rec->password, PASSWORD_LENGTH);
            tonghuashun_rec->action = 1;
            break;
        }

		return YT_FAILED;
	}while (0);

	//先获得下时间
	time(&tonghuashun_time);
	tonghuashun_time_now = localtime(&tonghuashun_time);	
	sprintf(tonghuashun_rec->date, "%04d-%02d-%02d", tonghuashun_time_now->tm_year + 1900, 
			tonghuashun_time_now->tm_mon + 1, tonghuashun_time_now->tm_mday);

	sprintf(tonghuashun_suffix, "%04d%02d%02d", tonghuashun_time_now->tm_year + 1900,
			tonghuashun_time_now->tm_mon + 1, tonghuashun_time_now->tm_mday);

	sprintf(tonghuashun_rec->time, "%02d:%02d:%02d", tonghuashun_time_now->tm_hour, 
			tonghuashun_time_now->tm_min, tonghuashun_time_now->tm_sec);

	plugin_data_storage(dtts, tonghuashun_rec, thread_num);


	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 tonghuashun_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct tonghuashun_record *tonghuashun_rec, const int thread_num)
{
	if(!dtts || !tonghuashun_rec){
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
	sprintf(table_name, "%s%s", STOCK_PRIFIX, tonghuashun_suffix);

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
			tonghuashun_rec->action,
			tonghuashun_rec->date,
			tonghuashun_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			tonghuashun_rec->account,
			tonghuashun_rec->password,
			tonghuashun_rec->device_type,
			tonghuashun_rec->os_ver
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "tonghuashun storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：tonghuashun_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct tonghuashun_record *tonghuashun_dst)
{	
	if(tonghuashun_dst){
		mempool_free(tonghuashun_dst);
	}
}
