/***********************************************************
*文件名：plugin_haofang.c
*创建人：xiaocai_mo
*日  期：2014年07月15日
*描  述：haofang插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_game_haofang.h"
#define PROTOCOLID "2999"
#define PROTOCOLSUBID "299900000004"
#define PROTOCOLNAME  "浩方对战平台"

//文件存放路径
static char *haofang_plugin_path = NULL;

//表名的时间后缀
static char haofang_suffix[COMMON_LENGTH_32] = {0};

//同花顺端口数组
static int GAME_PORT[1] = {1201};

#pragma pack(push)
#pragma pack(1)
struct haofanggame_protocol_t
{
    unsigned char fix_00;
    unsigned char cmd;
    unsigned int fix_0x2d000000;
    unsigned short serial;
    unsigned short fix_ffff;
    unsigned int haofangid;
    unsigned char data[0];
};
#pragma pack(pop)

/*
*函数名称：plugin_init
*函数功能：初始化haofang插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_haofang 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "haofang base_get_configuration() error\n");
		return YT_FAILED;
	}

	haofang_plugin_path = config_tmp->plugin_storage_path;

	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = GAME_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < GAME_PORT_NUM; i++){
		ppi.port = GAME_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "haofang plugin_manager_register() error\n");
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

	struct haofang_record *haofang_rec = NULL;

	haofang_rec = (struct haofang_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(haofang_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

    //判断是否是bbs处理后的后续杂包
    if(haofang_rec && haofang_rec->plugin_ds.plugin_type == PLUGIN_TYPE_BBS) {
        return YT_FAILED;
    }

	//haofang客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
        if (!haofang_rec) {
		    haofang_rec = (struct haofang_record*)mempool_malloc(sizeof(struct haofang_record));
		    if(!haofang_rec){
		    	log_write(LOG_ERROR, "haofang_rec malloc() error\n");
		    	return YT_FAILED;
		    }
		    dtts->deal_record->record = haofang_rec;
        }

		if(plugin_deal_client(dtts, haofang_rec, thread_num) == YT_FAILED) {
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
*输出参数：haofang_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct haofang_record *haofang_rec, const int thread_num)
{
	
	time_t haofang_time;
	struct tm *haofang_time_now;
     
    char *user_id = NULL;
    unsigned char feature[] = {0xaf, 0x01, 0x00, 0x00, 0x31, 0x03, 0x00, 0x00};

	if(!haofang_rec || dtts->l4_data_size < 319){
		return YT_FAILED;
	}

	do {
        // haofang game login
        if (!memcmp(dtts->l4_data, feature, 8)) {
            user_id = dtts->l4_data + 319;
            if (strlen(user_id) < ACCOUNT_LENGTH) {
                strcpy(haofang_rec->account, user_id);
            }
            haofang_rec->action = 1;
            break;
        }

		return YT_FAILED;
	}while (0);

	//先获得下时间
	time(&haofang_time);
	haofang_time_now = localtime(&haofang_time);	
	sprintf(haofang_rec->date, "%04d-%02d-%02d", haofang_time_now->tm_year + 1900, 
			haofang_time_now->tm_mon + 1, haofang_time_now->tm_mday);

	sprintf(haofang_suffix, "%04d%02d%02d", haofang_time_now->tm_year + 1900,
			haofang_time_now->tm_mon + 1, haofang_time_now->tm_mday);

	sprintf(haofang_rec->time, "%02d:%02d:%02d", haofang_time_now->tm_hour, 
			haofang_time_now->tm_min, haofang_time_now->tm_sec);

	plugin_data_storage(dtts, haofang_rec, thread_num);


	//匹配上了返回成功
	return YT_SUCCESSFUL;
	
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 haofang_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct haofang_record *haofang_rec, const int thread_num)
{
	if(!dtts || !haofang_rec){
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
	sprintf(table_name, "%s%s", GAME_PRIFIX, haofang_suffix);

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
			haofang_rec->action,
			haofang_rec->date,
			haofang_rec->time,
			element_tmp.sip,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dmac,
			element_tmp.dport,
			haofang_rec->account,
			haofang_rec->password,
			haofang_rec->device_type,
			haofang_rec->os_ver
			);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "haofang storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：haofang_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct haofang_record *haofang_dst)
{	
	if(haofang_dst){
		mempool_free(haofang_dst);
	}
}
