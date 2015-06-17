/***********************************************************
*文件名：plugin_im_yixin.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：yixin插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_im_yixin.h"

#define PROTOCOLID "1999"
#define PROTOCOLSUBID "199900000008"
#define PROTOCOLNAME  "易信"

//文件存放路径
static char *yixin_plugin_path = NULL;

//表名的时间后缀
static char yixin_suffix[COMMON_LENGTH_32] = {0};

//URL端口数组
static int URL_PORT[2] = {80,8080};

/*
*函数名称：plugin_init
*函数功能：初始化yixin插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_yixin 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "yixin base_get_configuration() error\n");
		return YT_FAILED;
	}

	yixin_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = IM_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < URL_PORT_NUM; i++){
		ppi.port = URL_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "yixin plugin_manager_register() error\n");
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

	struct yixin_record *yixin_rec = NULL;
	
	yixin_rec = (struct yixin_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(yixin_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是bbs处理后的后续杂包
    if(yixin_rec && yixin_rec->plugin_ds.plugin_type == PLUGIN_TYPE_BBS) {
        return YT_FAILED;
    }         

	//yixin客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		plugin_done(yixin_rec);
		(dtts->deal_record)->record = NULL;
	
		yixin_rec = (struct yixin_record*)mempool_malloc(sizeof(struct yixin_record));
		if(!yixin_rec){
			log_write(LOG_ERROR, "yixin_rec malloc() error\n");
			return YT_FAILED;
		}

		(dtts->deal_record)->record = yixin_rec;
		
		if(plugin_deal_client(dtts, yixin_rec, thread_num) == YT_FAILED) {
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
*输出参数：yixin_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct yixin_record *yixin_rec, const int thread_num)
{
	if(!yixin_rec){
		return YT_FAILED;
	}
	
	time_t yixin_time;
	struct tm *yixin_time_now;

	do {
		//iphone yixin登陆 识别
		if (!memcmp(YIXIN_MARK_HEAD_IPHONE,dtts->l4_data,YIXIN_MARK_HEAD_IPHONE_SIZE)) {
			if (strstr(dtts->l4_data+YIXIN_MARK_HEAD_IPHONE_SIZE,YIXIN_MARK_FEATURE_IPHONE)) {
//              获取不到账号
//				plugin_data_extract(dtts->l4_data, WEIXIN_EXTRACT_HEAD_ACCOUNT_IPHONE, 
//						            WEIXIN_EXTRACT_TAIL_ACCOUNT_IPHONE, yixin_rec->account, ACCOUNT_LENGTH);
				yixin_rec->action = 1;
				break;
			}
		}

		//安卓版输入密码方式账号获取
		if (!memcmp(YIXIN_MARK_HEAD_ANDROID,dtts->l4_data,YIXIN_MARK_HEAD_ANDROID_SIZE)) {
			if (strstr(dtts->l4_data+YIXIN_MARK_HEAD_ANDROID_SIZE,YIXIN_MARK_FEATURE_ANDROID)) {
//              获取到账号
				plugin_data_extract(dtts->l4_data, YIXIN_EXTRACT_HEAD_ACCOUNT_ANDROID, 
						            YIXIN_EXTRACT_TAIL_ACCOUNT_ANDROID, yixin_rec->account, ACCOUNT_LENGTH);
				yixin_rec->action = 1;
				break;
			}
		}

		plugin_done(yixin_rec);
		(dtts->deal_record)->record = NULL;
		return YT_FAILED;

	}while (0);


	//先获得下时间
	time(&yixin_time);
	yixin_time_now = localtime(&yixin_time);	
	sprintf(yixin_rec->date, "%04d-%02d-%02d", yixin_time_now->tm_year + 1900, 
			yixin_time_now->tm_mon + 1, yixin_time_now->tm_mday);

	sprintf(yixin_suffix, "%04d%02d%02d", yixin_time_now->tm_year + 1900,
			yixin_time_now->tm_mon + 1, yixin_time_now->tm_mday);

	sprintf(yixin_rec->time, "%02d:%02d:%02d", yixin_time_now->tm_hour, 
			yixin_time_now->tm_min, yixin_time_now->tm_sec);

	plugin_data_storage(dtts, yixin_rec, thread_num);

	plugin_done(yixin_rec);
	(dtts->deal_record)->record = NULL;

	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 yixin_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct yixin_record *yixin_rec, const int thread_num)
{
	if(!dtts || !yixin_rec){
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
	sprintf(table_name, "%s%s", IM_PRIFIX, yixin_suffix);

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
			yixin_rec->action,
			fcontent,
			yixin_rec->date,
			yixin_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			yixin_rec->account,
			yixin_rec->device_type,
			yixin_rec->os_ver
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "yixin storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：yixin_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct yixin_record *yixin_dst)
{	
	if(yixin_dst){
		mempool_free(yixin_dst);
	}

}
