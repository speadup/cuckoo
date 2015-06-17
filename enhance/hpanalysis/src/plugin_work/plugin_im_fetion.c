/***********************************************************
*文件名：plugin_im_fetion.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：fetion插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_im_fetion.h"

#define PROTOCOLID "1999"
#define PROTOCOLSUBID "199900000001"
#define PROTOCOLNAME  "飞信"

//文件存放路径
static char *fetion_plugin_path = NULL;

//表名的时间后缀
static char fetion_suffix[COMMON_LENGTH_32] = {0};

//URL端口数组
static int URL_PORT[2] = {80,8023};

/*
*函数名称：plugin_init
*函数功能：初始化fetion插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_fetion 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "fetion base_get_configuration() error\n");
		return YT_FAILED;
	}

	fetion_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = IM_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < URL_PORT_NUM; i++){
		ppi.port = URL_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "fetion plugin_manager_register() error\n");
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

	struct fetion_record *fetion_rec = NULL;
	fetion_rec = (struct fetion_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(fetion_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是该插件的数据包
    if(fetion_rec) {
		if (fetion_rec->plugin_ds.plugin_type != PLUGIN_TYPE_IM_FETION) {
			return YT_FAILED;
		}
    } else {
		fetion_rec = (struct fetion_record*)mempool_malloc(sizeof(struct fetion_record));
		if(!fetion_rec){
			log_write(LOG_ERROR, "fetion_rec malloc() error\n");
			return YT_FAILED;
		}

		fetion_rec->plugin_ds.plugin_type = PLUGIN_TYPE_IM_FETION;
		(dtts->deal_record)->record = fetion_rec;
	}

	//fetion客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		
		if(plugin_deal_client(dtts, fetion_rec, thread_num) == YT_FAILED) {
			return YT_FAILED;
		}

		return YT_SUCCESSFUL;
	}

	//fetion服务器端数据
	if((dtts->pkt_info).client_or_server == FROM_SERVER_DATA){
		
		if(plugin_deal_server(dtts, fetion_rec, thread_num) == YT_FAILED) {
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
*输出参数：fetion_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct fetion_record *fetion_rec, const int thread_num)
{
	if(!fetion_rec){
		return YT_FAILED;
	}
	
	time_t fetion_time;
	struct tm *fetion_time_now;

	do {
		//iphone fetion登陆 识别 android登陆识别与pc相同
		if (!memcmp(FETION_MARK_HEAD_IPHONE,dtts->l4_data,FETION_MARK_HEAD_IA_SIZE) ||
			!memcmp(FETION_MARK_HEAD_ANDROID,dtts->l4_data,FETION_MARK_HEAD_IA_SIZE)) {
			if (strstr(dtts->l4_data+FETION_MARK_HEAD_IA_SIZE,FETION_MARK_FEATURE_IA)) {
//              获取账号
				plugin_data_extract(dtts->l4_data, FETION_EXTRACT_HEAD_ACCOUNT_IA, 
						            FETION_EXTRACT_TAIL_ACCOUNT_IA, fetion_rec->account, ACCOUNT_LENGTH);
				fetion_rec->action = 1;
				break;
			}
		}

        const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);

	    if (ntohs(net_tuple_tmp->port_dest) == 8023) {
			//android 消息发送
		    if (!memcmp("font faceex",dtts->l4_data + 40, 11)) {
                //message
				plugin_data_extract(dtts->l4_data + 38, "<font faceex=\"\">", 
						            "</font>", fetion_rec->content, COMMON_LENGTH_512);
                //convert_encodes(fetion_rec->content, sizeof(fetion_rec->content), ENCODE_UTF8, NULL);
                char *p = strstr(dtts->l4_data + 38, "</font>");
                if (p) {
                    // send to num
    				plugin_data_extract(p, "sip:", 
    						            ";", fetion_rec->remote_account, ACCOUNT_LENGTH);
                }

				char *str = strstr(p, "sip:");
                if (str) {
                    // send to num
    				plugin_data_extract(str+19, "sip:", 
    						            ";", fetion_rec->account, ACCOUNT_LENGTH);
                }
				fetion_rec->action = 3;
				break;
			}

			//ios 消息发送
			char *mark = strstr(dtts->l4_data+27,"Font FaceEx=\"mrfont\">");
		    if (mark) {
                //message
				plugin_data_extract(dtts->l4_data + 27, "Font FaceEx=\"mrfont\">", 
						            "</Font>", fetion_rec->content, COMMON_LENGTH_512);
                //convert_encodes(fetion_rec->content, sizeof(fetion_rec->content), ENCODE_UTF8, NULL);
    			plugin_data_extract(dtts->l4_data+27,"sip:",";",fetion_rec->remote_account, ACCOUNT_LENGTH);
				fetion_rec->action = 3;
				break;
			}
		}

		plugin_done(fetion_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);

	//先获得下时间
	time(&fetion_time);
	fetion_time_now = localtime(&fetion_time);	
	sprintf(fetion_rec->date, "%04d-%02d-%02d", fetion_time_now->tm_year + 1900, 
			fetion_time_now->tm_mon + 1, fetion_time_now->tm_mday);

	sprintf(fetion_suffix, "%04d%02d%02d", fetion_time_now->tm_year + 1900,
			fetion_time_now->tm_mon + 1, fetion_time_now->tm_mday);

	sprintf(fetion_rec->time, "%02d:%02d:%02d", fetion_time_now->tm_hour, 
			fetion_time_now->tm_min, fetion_time_now->tm_sec);

	plugin_data_storage(dtts, fetion_rec, thread_num);

	plugin_done(fetion_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_deal_server
*函数功能：处理客户端数据
*输入参数：dtts:提供待处理的数据
*输出参数：fetion_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_server(const struct data_transit *dtts, struct fetion_record *fetion_rec, const int thread_num)
{
	if(!fetion_rec){
		return YT_FAILED;
	}
	
	time_t fetion_time;
	struct tm *fetion_time_now;

	do {
        const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);

	    if (ntohs(net_tuple_tmp->port_source) == 8023) {
			//android 消息接收
			char *mark = strstr(dtts->l4_data+70,"Font FaceEx=\"mrfont\">");
		    if (mark) {
                //message
				plugin_data_extract(dtts->l4_data + 70, "Font FaceEx=\"mrfont\">", 
						            "</Font>", fetion_rec->content, COMMON_LENGTH_512);
                //convert_encodes(fetion_rec->content, sizeof(fetion_rec->content), ENCODE_UTF8, NULL);
    			plugin_data_extract(dtts->l4_data+70,"sip:",";",fetion_rec->remote_account, ACCOUNT_LENGTH);
				fetion_rec->action = 4;
				break;
			}

			//ios 消息发送
			mark = strstr(dtts->l4_data+30,"font faceex=\"\">");
		    if (mark) {
                //message
				plugin_data_extract(dtts->l4_data + 30, "font faceex=\"\">", 
						            "</font>", fetion_rec->content, COMMON_LENGTH_512);
                //convert_encodes(fetion_rec->content, sizeof(fetion_rec->content), ENCODE_UTF8, NULL);
    			plugin_data_extract(dtts->l4_data+30,"sip:",";",fetion_rec->remote_account, ACCOUNT_LENGTH);
				fetion_rec->action = 4;
				break;
			}
		}

		plugin_done(fetion_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);

	//先获得下时间
	time(&fetion_time);
	fetion_time_now = localtime(&fetion_time);	
	sprintf(fetion_rec->date, "%04d-%02d-%02d", fetion_time_now->tm_year + 1900, 
			fetion_time_now->tm_mon + 1, fetion_time_now->tm_mday);

	sprintf(fetion_suffix, "%04d%02d%02d", fetion_time_now->tm_year + 1900,
			fetion_time_now->tm_mon + 1, fetion_time_now->tm_mday);

	sprintf(fetion_rec->time, "%02d:%02d:%02d", fetion_time_now->tm_hour, 
			fetion_time_now->tm_min, fetion_time_now->tm_sec);

	plugin_data_storage(dtts, fetion_rec, thread_num);

	plugin_done(fetion_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 fetion_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct fetion_record *fetion_rec, const int thread_num)
{
	if(!dtts || !fetion_rec){
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
	sprintf(table_name, "%s%s", IM_PRIFIX, fetion_suffix);

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
			fetion_rec->action,
			fetion_rec->content,
			fetion_rec->date,
			fetion_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			fetion_rec->account,
			fetion_rec->device_type,
			fetion_rec->os_ver,
            fetion_rec->remote_account
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "fetion storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：fetion_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct fetion_record *fetion_dst)
{	
	if(fetion_dst){
		mempool_free(fetion_dst);
	}

}
