/***********************************************************
*文件名：plugin_im_mi.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：mi插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_im_mi.h"

#define PROTOCOLID "1999"
#define PROTOCOLSUBID "199900000007"
#define PROTOCOLNAME  "米聊"

//文件存放路径
static char *mi_plugin_path = NULL;

//表名的时间后缀
static char mi_suffix[COMMON_LENGTH_32] = {0};

//URL端口数组
static int URL_PORT[2] = {80,8080};

/*
*函数名称：plugin_init
*函数功能：初始化mi插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_mi 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "mi base_get_configuration() error\n");
		return YT_FAILED;
	}

	mi_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = IM_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < URL_PORT_NUM; i++){
		ppi.port = URL_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "mi plugin_manager_register() error\n");
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

	struct mi_record *mi_rec = NULL;
	
	mi_rec = (struct mi_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(mi_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是bbs处理后的后续杂包
    if(mi_rec && mi_rec->plugin_ds.plugin_type == PLUGIN_TYPE_BBS) {
        return YT_FAILED;
    }         

	//mi客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		plugin_done(mi_rec);
		(dtts->deal_record)->record = NULL;
	
		mi_rec = (struct mi_record*)mempool_malloc(sizeof(struct mi_record));
		if(!mi_rec){
			log_write(LOG_ERROR, "mi_rec malloc() error\n");
			return YT_FAILED;
		}

		(dtts->deal_record)->record = mi_rec;
		
		if(plugin_deal_client(dtts, mi_rec, thread_num) == YT_FAILED) {
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
*输出参数：mi_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct mi_record *mi_rec, const int thread_num)
{
	if(!mi_rec){
		return YT_FAILED;
	}
	
	time_t mi_time;
	struct tm *mi_time_now;

	do {
		//iphone mi登陆  无法识别
/*		if (!memcmp(WEIXIN_MARK_HEAD_IPHONE,dtts->l4_data,WEIXIN_MARK_HEAD_IPHONE_SIZE)) {
			if (strstr(dtts->l4_data+WEIXIN_MARK_HEAD_IPHONE_SIZE,WEIXIN_MARK_FEATURE_IPHONE)) {
//              获取不到微信的账号
//				plugin_data_extract(dtts->l4_data, WEIXIN_EXTRACT_HEAD_ACCOUNT_IPHONE, 
//						            WEIXIN_EXTRACT_TAIL_ACCOUNT_IPHONE, mi_rec->account, ACCOUNT_LENGTH);
				mi_rec->action = 1;
				break;
			}
		}
*/
		//android mi登陆
		if (!memcmp("GET /v3/user",dtts->l4_data,12)) {
			if (strstr(dtts->l4_data+12,"111.206.200.6")) {
//              获取账号
				plugin_data_extract(dtts->l4_data, "uuid=", "&", mi_rec->account, ACCOUNT_LENGTH);
				mi_rec->action = 1;
				break;
			}
		}
	
		plugin_done(mi_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);


	//先获得下时间
	time(&mi_time);
	mi_time_now = localtime(&mi_time);	
	sprintf(mi_rec->date, "%04d-%02d-%02d", mi_time_now->tm_year + 1900, 
			mi_time_now->tm_mon + 1, mi_time_now->tm_mday);

	sprintf(mi_suffix, "%04d%02d%02d", mi_time_now->tm_year + 1900,
			mi_time_now->tm_mon + 1, mi_time_now->tm_mday);

	sprintf(mi_rec->time, "%02d:%02d:%02d", mi_time_now->tm_hour, 
			mi_time_now->tm_min, mi_time_now->tm_sec);

	plugin_data_storage(dtts, mi_rec, thread_num);

	plugin_done(mi_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 mi_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct mi_record *mi_rec, const int thread_num)
{
	if(!dtts || !mi_rec){
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
	sprintf(table_name, "%s%s", IM_PRIFIX, mi_suffix);

	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}

	char fcontent[70] = "\0";
    srand((unsigned int)time((time_t *)NULL));
	get_rand_str(fcontent,rand()%65);

	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%d','%s','%s %s','%s','%s','%d','%s','%s','%d','%s','%s','%s');",
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,faction,fcontent,fregtime,fsourceip,fsourcemac,fsourceport,fdestinationip,fdestinationmac,fdestinationport,faccount,fitem1,fitem2",
			PLACECODE,
			PLACENAME,
			PROTOCOLID,
			PROTOCOLSUBID,
			PROTOCOLNAME,
			mi_rec->action,
			fcontent,
			mi_rec->date,
			mi_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			mi_rec->account,
			mi_rec->device_type,
			mi_rec->os_ver
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "mi storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：mi_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct mi_record *mi_dst)
{	
	if(mi_dst){
		mempool_free(mi_dst);
	}

}
