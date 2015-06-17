/***********************************************************
*文件名：plugin_url.c
*创建人：韩涛
*日  期：2013年10月25日
*描  述：url插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_public.h"
#include "plugin_url.h"

static char *url_field_name[URL_FEATURE_FIELD_NUM] = {PLUGIN_ACCOUNT, PLUGIN_PASSWORD, PLUGIN_WEB, PLUGIN_URL};

//特征码链表头
static struct plugin_feature url_feature_head;

//文件存放路径
static char *url_plugin_path = NULL;

//表名的时间后缀
static char url_suffix[COMMON_LENGTH_32] = {0};

//URL端口数组
static int URL_PORT[2] = {80,8080};

/*
*函数名称：plugin_init
*函数功能：初始化url插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_url 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "url base_get_configuration() error\n");
		return YT_FAILED;
	}

	url_plugin_path = config_tmp->plugin_storage_path;

	INIT_LIST_HEAD(&(url_feature_head.list_node));

	//获取特征码
	if(feature_manager_get_feature(&url_feature_head, URL_GROUP_ID)){
		log_write(LOG_ERROR, "url feature_manager_get_feature() error\n");
		return YT_FAILED;
	}

	//初始化特征码
	plugin_feature_init();
	
	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = URL_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < URL_PORT_NUM; i++){
		ppi.port = URL_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "url plugin_manager_register() error\n");
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
*返回值： 无
*/
void plugin_feature_init()
{
	int i = 0;
	
	struct list_head *list_head_tmp = NULL;
	struct list_head *list_head_extract_tmp = NULL;
	struct plugin_feature *url_feature_tmp = NULL;
	struct plugin_feature_extract *url_fe = NULL;
	
	//循环遍历特征码
	list_for_each(list_head_tmp, &(url_feature_head.list_node)){
		url_feature_tmp = (struct plugin_feature*)list_head_tmp;
		//循环遍历截取特征码
		list_for_each(list_head_extract_tmp, &(url_feature_tmp->pextract_list_node)){
			url_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
			//匹配字段
			for(i = 0;i < URL_FEATURE_FIELD_NUM; i++){
				if(!memcmp(url_fe->field_name, url_field_name[i], strlen(url_field_name[i]))){
					//设置回调
					switch (i){	
						case PLUGIN_ACCOUNT_NUM:
							url_fe->excb = plugin_deal_account;
							break;
						case PLUGIN_PASSWORD_NUM:
							url_fe->excb = plugin_deal_password;
							break;
						case URL_WEB_NUM:
							url_fe->excb = plugin_deal_web;
							url_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case URL_URL_NUM:
							url_fe->excb = plugin_deal_url;
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
*返回值：0成功 -1失败
*/
static int plugin_deal(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts){
		return YT_FAILED;
	}

	struct plugin_deal_status *pds = NULL;
	struct url_record *url_rec = NULL;
	
	url_rec = (struct url_record*)((dtts->deal_record)->record);	
	
	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		//超时了或关闭连接了，服务端响应还没来也不管了
		plugin_done(url_rec);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;

		return YT_SUCCESSFUL;
	}

	//判断是否为其他插件中的url
	if(url_rec){
		if (url_rec->plugin_ds.plugin_type != PLUGIN_TYPE_URL) {
			return YT_FAILED;
		}
	} else {
		url_rec = (struct url_record*)mempool_malloc(sizeof(struct url_record));
		if(!url_rec){
			log_write(LOG_ERROR, "url_dst malloc() error\n");
			return YT_FAILED;
		}

		url_rec->plugin_ds.plugin_type = PLUGIN_TYPE_URL;
		(dtts->deal_record)->record = url_rec;
	}

	//url客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		
		//客户端数据直接进行保存，等待服务端数据到来之后进行判读
		//看是否应该对数据进行处理
		if(plugin_url_storage(url_rec, dtts, thread_num)){
			log_write(LOG_ERROR, "url plugin_url_storage() error\n");
			return YT_FAILED;
		}
	
		return YT_SUCCESSFUL;
	}

	//url服务端数据
	if((dtts->pkt_info).client_or_server == FROM_SERVER_DATA){

		if(url_rec && url_rec->deal_flag == DATA_NO_DEAL){
			if(plugin_deal_server(dtts, thread_num)){
				return YT_FAILED;			
			}
		}
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_url_storage
*函数功能：存储客户端数据
*输入参数：dtts：待存储数据结构体 thread_num: 线程号 
*输出参数：无
*返回值：  0成功 -1失败
*/
static int plugin_url_storage(struct url_record *url_rec, const struct data_transit *dtts, const int thread_num)
{
	if(!url_rec || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}
	
	memcpy(url_rec->url_data, dtts->l4_data, MIN(URL_STORAGE_LEN, dtts->l4_data_size));

	url_rec->url_data_size = dtts->l4_data_size;
	url_rec->thread_num = thread_num;
	url_rec->deal_flag = DATA_NO_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*输入参数：dtts:提供待处理的数据
*输出参数：url_rec:存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct url_record *url_rec, const int thread_num)
{
	if(!url_rec || !(url_rec->url_data_size)){
		return YT_FAILED;
	}

	struct plugin_feature *marked_feature = NULL;
	time_t url_time;
	struct tm *url_time_now;

	//只匹配一个进行处理
	marked_feature = plugin_feature_mark(url_rec->url_data, url_rec->url_data_size, &url_feature_head);

	//如果匹配上了url
	if(marked_feature){
	
		//先获得下时间
		time(&url_time);
		url_time_now = localtime(&url_time);	
		sprintf((url_rec->url_req).date, "%04d-%02d-%02d", url_time_now->tm_year + 1900, 
				url_time_now->tm_mon + 1, url_time_now->tm_mday);

		sprintf(url_suffix, "%04d%02d%02d", url_time_now->tm_year + 1900,
				url_time_now->tm_mon + 1, url_time_now->tm_mday);

		sprintf((url_rec->url_req).time, "%02d:%02d:%02d", url_time_now->tm_hour, 
				url_time_now->tm_min, url_time_now->tm_sec);
	
		//存储匹配上特征码结构体，为入库提供信息
		(url_rec->url_req).url_feature = marked_feature;

		//然后进行数据的截取
		if(plugin_feature_extract(url_rec->url_data, url_rec->url_data_size, &(marked_feature->pextract_list_node), &(url_rec->url_req))){
	//		log_write(LOG_ERROR, "url plugin_feature_extract error");
		}

		//截取完了进行存储
		if(UNLIKELY(plugin_data_storage(dtts, url_rec, thread_num))){
	//		log_write(LOG_ERROR, "url plugin_data_storage() error\n");
		}

		url_rec->deal_flag = DATA_DEAL;
		
		plugin_done(url_rec);
		(dtts->deal_record)->record = NULL;

		//匹配上了返回成功
		return YT_SUCCESSFUL;
	}
	
	return YT_FAILED;
}

/*
*函数名称：plugin_deal_server
*函数功能：处理服务端数据
*输入参数：dtts：待处理数据 
*输出参数：无
*返回值：  0成功 -1失败
*/
static int plugin_deal_server(const struct data_transit *dtts, const int thread_num)
{
	struct url_record *url_rec = (struct url_record*)(dtts->deal_record)->record;
	
	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size) || !url_rec){
		return YT_FAILED;
	}	

	char *p_head_tmp = NULL;
	char *p_head = NULL;
	char *p_tail = NULL;
	char type_tmp[COMMON_LENGTH_32] = {0};

	if((dtts->l4_data_size) < HTTP_STATUS_HEAD_LEN)
	{
		return YT_FAILED;
	}
	p_head_tmp = dtts->l4_data + HTTP_STATUS_INDEX;
	
	if(!memcmp(p_head_tmp, RESPONSE_STATUS_200, HTTP_SATTUS_LENGTH) || 
			!memcmp(p_head_tmp, RESPONSE_STATUS_304, HTTP_SATTUS_LENGTH)){
		do{
			if(!memcmp(p_head_tmp, URL_TYPE_TITLE, strlen(URL_TYPE_TITLE))){
				p_head_tmp = p_head_tmp + strlen(URL_TYPE_TITLE);
				
				p_tail = strstr(p_head_tmp, PLUGIN_TAIL);
		
				if(p_tail){
					memcpy(type_tmp, p_head_tmp, p_tail - p_head_tmp);
				}
				else{
					return YT_FAILED;
				}

				//如果找到Content-Type并且找到html或xml，那么我就对数据进行处理	
				if(strcasestr(type_tmp, URL_TYPE_HTML) || strcasestr(type_tmp, URL_TYPE_XML)){
					return plugin_deal_client(dtts, url_rec, thread_num);
				}
				else{	
					//找到Content-Type但不是我想要的类型
					return YT_FAILED;
				}
			}

			p_head = strstr(p_head_tmp, PLUGIN_TAIL);
			if(p_head){
				p_head_tmp = p_head + strlen(PLUGIN_TAIL);
			}
			else 
				break;

		}while(memcmp(p_head_tmp, PLUGIN_TAIL, strlen(PLUGIN_TAIL)) && global_shared.continue_run);	

	}

	return YT_FAILED;
}

/*
*函数名称：plugin_deal_account
*函数功能：处理account
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct url_request *dst_tmp = (struct url_request*)dst;
	
	if(plugin_extract_type_deal(deal_data, dst_tmp->account, sizeof(dst_tmp->account), plugin_fe)){
		return YT_FAILED;
	}
	
	convert_encodes(dst_tmp->account, sizeof(dst_tmp->account), plugin_fe->encode, NULL);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_password
*函数功能：处理password
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct url_request *dst_tmp = (struct url_request*)dst;
	
	if(plugin_extract_type_deal(deal_data, dst_tmp->password, sizeof(dst_tmp->password), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->password, sizeof(dst_tmp->password), plugin_fe->encode, NULL);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_web
*函数功能：处理web
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_web(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct url_request *dst_tmp = (struct url_request*)dst;

	if(plugin_extract_type_deal(deal_data, dst_tmp->web, sizeof(dst_tmp->web), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->web, sizeof(dst_tmp->web), plugin_fe->encode, NULL);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_url
*函数功能：处理url
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_url(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	//过滤操作
	struct url_request *dst_tmp = (struct url_request*)dst;

	if(plugin_extract_type_deal(deal_data, dst_tmp->url, sizeof(dst_tmp->url), plugin_fe)){
		return YT_FAILED;
	}
	
	convert_encodes(dst_tmp->url, sizeof(dst_tmp->url), plugin_fe->encode, NULL);
	
	return YT_SUCCESSFUL;
}



/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：dtts:提供元组数据 url_dst：数据结构体 
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct url_record *url_rec, const int thread_num)
{
	if(!dtts || !url_rec){
		return YT_FAILED;
	}
		
	struct url_request *url_dst = &(url_rec->url_req);
	struct plugin_element element_tmp;

	memset(&element_tmp, 0, sizeof(struct plugin_element));

	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);
	//	vlan_id
	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	//table name
	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", URL_PRIFIX, url_suffix);

	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}

	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%s','%s%s','%d','%s %s','%s','%s','%s','%d','%s','%s','%s','%d','%s','%s','%s');",
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,fweb,furl,faction,fregtime,fsourceip,fsourceipv6,fsourcemac,fsourceport,fdestinationip,fdestinationipv6,fdestinationmac,fdestinationport,faccount,fpassword,fsessionid",
			place_info[0],
			place_info[1],
			url_dst->url_feature->fprotocolid,
			url_dst->url_feature->fprotocolsubid,
			url_dst->url_feature->fprotocolname,
			url_dst->web, 
			url_dst->web,
			url_dst->url,
			url_dst->url_feature->action,
			url_dst->date,
			url_dst->time,
			element_tmp.sip,
			element_tmp.sip6,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dip6,
			element_tmp.dmac,
			element_tmp.dport,
			url_dst->account,
			url_dst->password,
			url_dst->sessionID
				);

	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "url storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：url_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct url_record *url_dst)
{	
	if(url_dst){
		mempool_free(url_dst);
	}

}
