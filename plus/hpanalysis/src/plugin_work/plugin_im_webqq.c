/***********************************************************
*文件名：plugin_im_webqq.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：webqq插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_im_webqq.h"

#define PROTOCOLID "1002"
#define PROTOCOLSUBID "100200000000"
#define PROTOCOLNAME  "webqq"

//文件存放路径
static char *webqq_plugin_path = NULL;

//表名的时间后缀
static char webqq_suffix[COMMON_LENGTH_32] = {0};

//URL端口数组
static int URL_PORT[1] = {80};

/*
*函数名称：plugin_init
*函数功能：初始化webqq插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_webqq 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "webqq base_get_configuration() error\n");
		return YT_FAILED;
	}

	webqq_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = IM_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < 1; i++){
		ppi.port = URL_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "webqq plugin_manager_register() error\n");
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

	struct webqq_record *webqq_rec = NULL;
	webqq_rec = (struct webqq_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(webqq_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是该插件的数据包
    if(webqq_rec) {
		if (webqq_rec->plugin_ds.plugin_type != PLUGIN_TYPE_IM_WEBQQ) {
			return YT_FAILED;
		}
    } else {
		webqq_rec = (struct webqq_record*)mempool_malloc(sizeof(struct webqq_record));
		if(!webqq_rec){
			log_write(LOG_ERROR, "webqq_rec malloc() error\n");
			return YT_FAILED;
		}

		webqq_rec->plugin_ds.plugin_type = PLUGIN_TYPE_IM_WEBQQ;
		(dtts->deal_record)->record = webqq_rec;
	}

	//webqq客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){

		if(plugin_deal_client(dtts, webqq_rec, thread_num) == YT_FAILED) {
			return YT_FAILED;
		}

		return YT_SUCCESSFUL;
	}

	//webqq服务器端数据
	if((dtts->pkt_info).client_or_server == FROM_SERVER_DATA){
	
		if(plugin_deal_server(dtts, webqq_rec, thread_num) == YT_FAILED) {
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
*输出参数：webqq_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct webqq_record *webqq_rec, const int thread_num)
{
	if(!webqq_rec){
		return YT_FAILED;
	}
	
	time_t webqq_time;
	struct tm *webqq_time_now;

	do {
		//webqq登陆 识别
		if (!memcmp("GET /check_sig",dtts->l4_data,14)) {
			if (strstr(dtts->l4_data+14,"ptlogin4.web2.qq.com")) {
//              获取账号
				plugin_data_extract(dtts->l4_data,"uin=","&", webqq_rec->account, ACCOUNT_LENGTH);
				webqq_rec->action = 1;
				break;
			}
		}

        const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);

		//消息发送
	    if (!memcmp("POST /channel/send_buddy_msg2",dtts->l4_data, 29)) {
            //message
			plugin_data_extract(dtts->l4_data + 200, "content%22%3A%22%5B%5C%22", 
					            "%5C%22%2C%5B%5C%22", webqq_rec->content, COMMON_LENGTH_512);

            convert_encodes(webqq_rec->content, strlen(webqq_rec->content), "1", NULL);

			plugin_data_extract(dtts->l4_data + 100, "ptui_loginuin=", 
					            ";",webqq_rec->account, ACCOUNT_LENGTH);
			webqq_rec->action = 3;
			break;
		}

		plugin_done(webqq_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);

	//先获得下时间
	time(&webqq_time);
	webqq_time_now = localtime(&webqq_time);	
	sprintf(webqq_rec->date, "%04d-%02d-%02d", webqq_time_now->tm_year + 1900, 
			webqq_time_now->tm_mon + 1, webqq_time_now->tm_mday);

	sprintf(webqq_suffix, "%04d%02d%02d", webqq_time_now->tm_year + 1900,
			webqq_time_now->tm_mon + 1, webqq_time_now->tm_mday);

	sprintf(webqq_rec->time, "%02d:%02d:%02d", webqq_time_now->tm_hour, 
			webqq_time_now->tm_min, webqq_time_now->tm_sec);

	plugin_data_storage(dtts, webqq_rec, thread_num);

	plugin_done(webqq_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_deal_server
*函数功能：处理客户端数据
*输入参数：dtts:提供待处理的数据
*输出参数：webqq_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_server(const struct data_transit *dtts, struct webqq_record *webqq_rec, const int thread_num)
{
	if(!webqq_rec){
		return YT_FAILED;
	}
	
	time_t webqq_time;
	struct tm *webqq_time_now;

	do {
		//消息接收
		char *mark = strstr(dtts->l4_data,"\"message\"");
	    if (mark) {
			char *mark1 = strstr(mark,"\"name\"");
			if (!mark1) {
				return YT_FAILED;
			}

            //message
			plugin_data_extract(mark1, ",\">", "\"", webqq_rec->content, COMMON_LENGTH_512);
            //convert_encodes(webqq_rec->content, sizeof(webqq_rec->content), ENCODE_UTF8, NULL);
			webqq_rec->action = 4;
			break;
		}

		plugin_done(webqq_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);

	//先获得下时间
	time(&webqq_time);
	webqq_time_now = localtime(&webqq_time);	
	sprintf(webqq_rec->date, "%04d-%02d-%02d", webqq_time_now->tm_year + 1900, 
			webqq_time_now->tm_mon + 1, webqq_time_now->tm_mday);

	sprintf(webqq_suffix, "%04d%02d%02d", webqq_time_now->tm_year + 1900,
			webqq_time_now->tm_mon + 1, webqq_time_now->tm_mday);

	sprintf(webqq_rec->time, "%02d:%02d:%02d", webqq_time_now->tm_hour, 
			webqq_time_now->tm_min, webqq_time_now->tm_sec);

	plugin_data_storage(dtts, webqq_rec, thread_num);

	plugin_done(webqq_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 webqq_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct webqq_record *webqq_rec, const int thread_num)
{
	if(!dtts || !webqq_rec){
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
	sprintf(table_name, "%s%s", IM_PRIFIX, webqq_suffix);

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

	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%d','%s','%s %s','%s','%s','%d','%s','%s','%d','%s','%s','%s', '%s');",
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,faction,fcontent,fregtime,fsourceip,fsourcemac,fsourceport,fdestinationip,fdestinationmac,fdestinationport,faccount,fitem1,fitem2, fremoteaccount",
			PLACECODE,
			PLACENAME,
			PROTOCOLID,
			PROTOCOLSUBID,
			PROTOCOLNAME,
			webqq_rec->action,
			webqq_rec->content,
			webqq_rec->date,
			webqq_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			webqq_rec->account,
			webqq_rec->device_type,
			webqq_rec->os_ver,
            webqq_rec->remote_account
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "webqq storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：webqq_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct webqq_record *webqq_dst)
{	
	if(webqq_dst){
		mempool_free(webqq_dst);
	}

}
