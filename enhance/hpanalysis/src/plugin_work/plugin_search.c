/***********************************************************
*文件名：plugin_search.c
*创建人：韩涛
*日  期：2013年10月25日
*描  述：search插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_public.h"
#include "plugin_search.h"

static char *search_field_name[SEARCH_FEATURE_FIELD_NUM] = {PLUGIN_SEARCHWORD};

//特征码链表头
static struct plugin_feature search_feature_head;
//文件存放路径
static char *search_plugin_path = NULL;

//表名前缀
static char search_suffix[COMMON_LENGTH_32] = {0};

//SEARCH端口数组
static int SEARCH_PORT[2]={80,8080};

/*
*函数名称：plugin_init
*函数功能：初始化search插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_search 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "search base_get_configuration() error\n");
		return YT_FAILED;
	}

	search_plugin_path = config_tmp->plugin_storage_path;
	
	INIT_LIST_HEAD(&(search_feature_head.list_node));

	//获取特征码
	if(feature_manager_get_feature(&search_feature_head, SEARCH_GROUP_ID)){
		log_write(LOG_ERROR, "search feature_manager_get_feature() error\n");
		return YT_FAILED;
	}

	//初始化特征码
	plugin_feature_init();
	
	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = SEARCH_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < SEARCH_PORT_NUM; i++){
		ppi.port = SEARCH_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "search plugin_manager_register() error\n");
			return YT_FAILED;
		}
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_feature_init
*函数功能：特征码初始化
*输入参数：无
*输出参数：无
*返回值：` 无
*/
void plugin_feature_init()
{
	int i = 0;
	
	struct list_head *list_head_tmp = NULL;
	struct list_head *list_head_extract_tmp = NULL;
	struct plugin_feature *search_feature_tmp = NULL;
	struct plugin_feature_extract *search_fe = NULL;
	
	//循环遍历特征码
	list_for_each(list_head_tmp, &(search_feature_head.list_node)){
		search_feature_tmp = (struct plugin_feature*)list_head_tmp;
		//循环遍历截取特征码
		list_for_each(list_head_extract_tmp, &(search_feature_tmp->pextract_list_node)){
			search_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
			//匹配字段
			for(i = 0;i < SEARCH_FEATURE_FIELD_NUM; i++){
				if(!memcmp(search_fe->field_name, search_field_name[i], strlen(search_field_name[i]))){
					//加回调
					switch (i){	
						case SEARCH_SEARCHWORD_NUM:
							search_fe->excb = plugin_deal_searchword;
							break;
						default:
							break;
					}
				}
			}
		}
	}
}
/*
函数名称：plugin_deal
函数功能：处理数据包
输入参数：dtts：待处理的数据结构体
		  thread_num：处理的线程号，用于存储模块
*输出参数：无
*返回值： 0成功 -1失败
*/
static int plugin_deal(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts){
		return YT_FAILED;
	}
	
	struct plugin_deal_status *pds = NULL;
	struct search_request *search_dst = NULL;
	
	search_dst = (dtts->deal_record)->record;
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		plugin_done(search_dst);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;
		return YT_SUCCESSFUL;
	}

	//判断是否是bbs处理后的后续杂包
	if(search_dst){
		pds = (struct plugin_deal_status*)(search_dst);
		if(pds && pds->plugin_type == PLUGIN_TYPE_BBS){
			//因为还有超时包需要往后仍,search的话可能多个因此不能返回
			return YT_FAILED;
		}
	}

	//search客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		if(plugin_deal_client(dtts, thread_num)){
			return YT_FAILED;
		}
		else{
			return YT_SUCCESSFUL;
		}
	}

	return YT_FAILED;
}

/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*输入参数：dtts：待处理数据长度 thread_num: 线程号
*输出参数：
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}
	
	struct search_request *search_dst = NULL;
	struct plugin_feature *marked_feature = NULL;
	time_t search_time;
	struct tm *search_time_now;

	//只匹配一个进行处理
	marked_feature = plugin_feature_mark(dtts->l4_data, dtts->l4_data_size, &search_feature_head);
	
	//只要匹配上了就返回成功，这样数据机会释放。不会传给下边的插件
	//所以你可能看到，错误了还返回success，这就是原因
	if(marked_feature){
	
		search_dst = (struct search_request*)((dtts->deal_record)->record);
		
		//分配过了，是URL的。结构体不同，要重新分配的
		if(search_dst){
			plugin_done(search_dst);
			(dtts->deal_record)->record = NULL;
		}
			
		search_dst = (struct search_request*)mempool_malloc(sizeof(struct search_request));
		if(!search_dst){
			log_write(LOG_ERROR, "search (dtts->session_deal)->record_data malloc() error\n");
			return YT_SUCCESSFUL;
		}

		(dtts->deal_record)->record = search_dst;

		//可能两次search在一个连接中，要重新清空
//		memset(search_dst, 0, sizeof(struct search_request));

		(search_dst->plugin_ds).plugin_type = PLUGIN_TYPE_SEARCH;

		search_dst->search_feature = marked_feature;
	
		//先获得下时间
		time(&search_time);
		search_time_now = localtime(&search_time);	
		sprintf(search_dst->date, "%04d-%02d-%02d", search_time_now->tm_year + 1900, 
				search_time_now->tm_mon + 1, search_time_now->tm_mday);

		sprintf(search_suffix, "%04d%02d%02d", search_time_now->tm_year + 1900,
				search_time_now->tm_mon + 1, search_time_now->tm_mday);

		sprintf(search_dst->time, "%02d:%02d:%02d", search_time_now->tm_hour, 
				search_time_now->tm_min, search_time_now->tm_sec);

		if(plugin_feature_extract(dtts->l4_data, dtts->l4_data_size, &(marked_feature->pextract_list_node), search_dst)){
	//		log_write(LOG_ERROR, "search plugin_feature_extract error\n");
		}
		
		//截取URL的话，添加特征码很麻烦，这是写死的处理。
		plugin_deal_url(dtts->l4_data, search_dst);

		//截取没成功起码把动作入库
		if(plugin_data_storage(dtts, search_dst, thread_num, PLUGIN_SEARCH_ID)){
	//		log_write(LOG_ERROR, "search plugin_data_storage() error\n");
		}
	
		return YT_SUCCESSFUL;
	}
	
	return YT_FAILED;
}

/*
*函数名称：plugin_deal_searchword
*函数功能：处理searchword
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_searchword(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct search_request *dst_tmp = (struct search_request*)dst;

	if(plugin_extract_type_deal(deal_data, dst_tmp->searchword, sizeof(dst_tmp->searchword), plugin_fe)){
		return YT_FAILED;
	}
	
	convert_encodes(dst_tmp->searchword, sizeof(dst_tmp->searchword), plugin_fe->encode, NULL);

	return YT_SUCCESSFUL;
}


/*
*函数名称：plugin_deal_url
*函数功能：处理url
*输入参数：deal_data：邮件内容数据 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_url(char *deal_data, void *dst)
{
	char web_tmp[COMMON_LENGTH_512 + 1] = {0};
	char url_tmp[URL_LENGTH + 1] = {0};
	char *head_tmp = NULL;
	char *tail_tmp = NULL;

	struct search_request *dst_tmp = (struct search_request*)dst;

	if(!memcmp(deal_data, PLUGIN_GET, strlen(PLUGIN_GET))){
		tail_tmp = strstr(deal_data, PLUGIN_HTTP);
		if(tail_tmp){
			head_tmp = deal_data + strlen(PLUGIN_GET);
			memcpy(url_tmp, head_tmp, tail_tmp - head_tmp);	
		}
		
	}
	else{
		return YT_FAILED;
	}
	
	head_tmp = strstr(deal_data, PLUGIN_HOST);
	if(head_tmp){
		tail_tmp = strstr(head_tmp, PLUGIN_TAIL);
		if(tail_tmp){
			head_tmp = head_tmp + strlen(PLUGIN_HOST);
			memcpy(web_tmp, head_tmp, tail_tmp - head_tmp);
		}
	}
	else{
		return YT_FAILED;
	}

	sprintf(dst_tmp->url, "%s%s", web_tmp, url_tmp);

	return YT_SUCCESSFUL;
}

/*
 *函数名称：plugin_data_storage
 *函数功能：拼接字符串，调用存储模块的写函数
 *输入参数：search_dst：数据结构体 thread_num：线程号 plugin_id:插件id
 *输出参数；无
 *返回值：0 成功 -1 失败
 */
static int plugin_data_storage(const struct data_transit *dtts, struct search_request *search_dst, const int thread_num, const int plugin_id)
{
	struct plugin_element element_tmp;
	memset(&element_tmp, 0, sizeof(struct plugin_element));

	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);
	//	vlan_id
	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", SEARCH_PRIFIX, search_suffix);


	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}   


	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%s','%s %s','%s','%s','%s','%d','%s','%s','%s','%d','1','%s');", 
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,fsearchword,fregtime,fsourceip,fsourceipv6,fsourcemac,fsourceport,fdestinationip,fdestinationipv6,fdestinationmac,fdestinationport,faction,furl",
			place_info[0],
			place_info[1],
			search_dst->search_feature->fprotocolid,
			search_dst->search_feature->fprotocolsubid,
			search_dst->search_feature->fprotocolname,
			search_dst->searchword,
			search_dst->date,
			search_dst->time,
			element_tmp.sip,
			element_tmp.sip6,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dip6,
			element_tmp.dmac,
			element_tmp.dport,
			search_dst->url
				);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "search storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：search_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(void *search_dst)
{	
	if(search_dst){
		mempool_free(search_dst);
	}
}
