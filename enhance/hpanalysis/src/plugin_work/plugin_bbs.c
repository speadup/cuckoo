/***********************************************************
*文件名：plugin_bbs.c
*创建人：韩涛
*日  期：2013年10月25日
*描  述：bbs插件处理相关
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE
#include "plugin_bbs.h"

static char *bbs_field_name[BBS_FEATURE_FIELD_NUM] = {PLUGIN_ACCOUNT, PLUGIN_PASSWORD, PLUGIN_TITLE, PLUGIN_CONTENT, PLUGIN_SUBBBS};

//特征码链表头
static struct plugin_feature bbs_feature_head;
//文件存放路径
static char *bbs_plugin_path = NULL;
//表名的前缀
static char bbs_suffix[COMMON_LENGTH_32] = {0};

//BBS端口数组
static int BBS_PORT[2] = {80, 8080};

/*
*函数名称：plugin_init
*函数功能：初始化bbs插件
*调用函数：插件管理模块的插件注册函数、获取特征码函数
*输入参数：无
*输出参数：0 成功 -1错误
*/
//__attribute ((constructor)) int plugin_init()
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_bbs 模块初始化\n");
	struct plugin_info ppi;
	int i = 0;
	struct yt_config *config_tmp = NULL;

	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "bbs base_get_configuration() error\n");
		return YT_FAILED;
	}
	bbs_plugin_path = config_tmp->plugin_storage_path;
	
	INIT_LIST_HEAD(&(bbs_feature_head.list_node));

	//获取特征码
	if(feature_manager_get_feature(&bbs_feature_head, BBS_GROUP_ID)){
		log_write(LOG_ERROR, "bbs feature_manager_get_feature() error\n");
		return YT_FAILED;
	}

	//初始化特征码
	plugin_feature_init();
	
	//注册插件
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = BBS_GROUP_ID;
	ppi.func_plugin_deal = plugin_deal;
	for(i = 0; i < BBS_PORT_NUM; i++){
		ppi.port = BBS_PORT[i];
	    if(plugin_manager_register(&ppi)){
			log_write(LOG_ERROR, "bbs plugin_manager_register() error\n");
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
	struct plugin_feature *bbs_feature_tmp = NULL;
	struct plugin_feature_extract *bbs_fe = NULL;
	
	//遍历整个特征码
	list_for_each(list_head_tmp, &(bbs_feature_head.list_node)){
		bbs_feature_tmp = (struct plugin_feature*)list_head_tmp;
		//遍历整个截取特征码
		list_for_each(list_head_extract_tmp, &(bbs_feature_tmp->pextract_list_node)){
			bbs_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
			//一次比较字段名
			for(i = 0;i < BBS_FEATURE_FIELD_NUM; i++){
				if(!memcmp(bbs_fe->field_name, bbs_field_name[i], strlen(bbs_field_name[i]))){
					//设置回调
					switch (i){	
						case PLUGIN_ACCOUNT_NUM:
							bbs_fe->excb = plugin_deal_account;
							break;
						case PLUGIN_PASSWORD_NUM:
							bbs_fe->excb = plugin_deal_password;
							break;
						case BBS_TITLE_NUM:
							bbs_fe->excb = plugin_deal_title;
							break;
						case BBS_CONTENT_NUM:
							bbs_fe->excb = plugin_deal_content;
							break;
						case BBS_SUBBBS_NUM:
							bbs_fe->excb = plugin_deal_subbbs;
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
	
	struct bbs_request *bbs_req = NULL;

	bbs_req = (dtts->deal_record)->record;

	//连接关闭或超时存储
	if(dtts->session_status == SESSION_STATUS_CLOSE || dtts->session_status == SESSION_STATUS_TIMEOUT){
		plugin_done(bbs_req);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;
		
		return YT_SUCCESSFUL;
	}

	//判断数据包类型
	if(bbs_req){
        switch(bbs_req->plugin_ds.plugin_type) {
            case PLUGIN_TYPE_SEARCH:
                return YT_FAILED;
                break;
            case PLUGIN_TYPE_BBS:
	            if(dtts->pkt_info.client_or_server == FROM_CLIENT_DATA &&
                    bbs_req->deal_flag == DATA_NO_DEAL){
                    if(plugin_bbs_storage(bbs_req, dtts, thread_num)){
                        log_write(LOG_ERROR, "bbs plugin_bbs_storage() error\n");
                        return YT_FAILED;
                    }

                    return YT_SUCCESSFUL;
                }

	            if(dtts->pkt_info.client_or_server == FROM_SERVER_DATA &&
                    bbs_req->deal_flag == DATA_NO_DEAL){
                    if(plugin_deal_client(dtts, thread_num)){
                        return YT_FAILED;
                    }

                    return YT_SUCCESSFUL;
                }
                break;
            default:
                return YT_FAILED;
                break;
        }
    } else {
	    if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA &&
	        plugin_feature_mark(dtts->l4_data, dtts->l4_data_size, &bbs_feature_head)) {
            bbs_req = (struct bbs_record*)mempool_malloc(sizeof(struct bbs_request));
            if(!bbs_req){
                log_write(LOG_ERROR, "bbs_request malloc() error\n");
                return YT_FAILED;
            }
            (dtts->deal_record)->record = bbs_req;
            bbs_req->plugin_ds.plugin_type = PLUGIN_TYPE_BBS;
            
            if(plugin_bbs_storage(bbs_req, dtts, thread_num)){
                log_write(LOG_ERROR, "bbs plugin_bbs_storage() error\n");
                return YT_FAILED;
            }

            return YT_SUCCESSFUL;
        }
    }

	return YT_FAILED;
}

/*
*函数名称：plugin_bbs_storage
*函数功能：存储客户端数据
*输入参数：dtts：待存储数据结构体 thread_num: 线程号 
*输出参数：无
*返回值：  0成功 -1失败
*/
static int plugin_bbs_storage(struct bbs_request *bbs_req, const struct data_transit *dtts, const int thread_num)
{
	if(!bbs_req || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}
	
	memcpy(bbs_req->bbs_data + bbs_req->bbs_data_size, dtts->l4_data,
            MIN(COMMON_LENGTH_4096 - bbs_req->bbs_data_size, dtts->l4_data_size) - 1);

	bbs_req->bbs_data_size += dtts->l4_data_size;
	bbs_req->thread_num = thread_num;
	bbs_req->deal_flag = DATA_NO_DEAL;

	return YT_SUCCESSFUL;
}
/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*调用函数：匹配函数、截取函数
*输入参数：dtts：待处理数据 thread_num: 线程号 
*输出参数：dtts->bbs_req: 存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}

	struct bbs_request *bbs_req = NULL;
	struct plugin_feature *marked_feature = NULL;
	time_t bbs_time;
	struct tm *bbs_time_now;
	
	bbs_req = (struct bbs_request*)((dtts->deal_record)->record);
    if (!bbs_req) {
		return YT_FAILED;
    }

	//只匹配一个进行处理
	marked_feature = plugin_feature_mark(bbs_req->bbs_data, bbs_req->bbs_data_size, &bbs_feature_head);
	if(marked_feature){
		(bbs_req->plugin_ds).plugin_type = PLUGIN_TYPE_BBS;

		bbs_req->bbs_feature = marked_feature;

		//先获得下时间
		time(&bbs_time);
		bbs_time_now = localtime(&bbs_time);	
		sprintf(bbs_req->date, "%04d-%02d-%02d", bbs_time_now->tm_year + 1900, 
				bbs_time_now->tm_mon + 1, bbs_time_now->tm_mday);

		sprintf(bbs_suffix, "%04d%02d%02d", bbs_time_now->tm_year + 1900,
				bbs_time_now->tm_mon + 1, bbs_time_now->tm_mday);

		sprintf(bbs_req->time, "%02d:%02d:%02d", bbs_time_now->tm_hour, 
				bbs_time_now->tm_min, bbs_time_now->tm_sec);
		

		if(plugin_feature_extract(bbs_req->bbs_data, bbs_req->bbs_data_size, &(marked_feature->pextract_list_node), bbs_req)){
	//		log_write(LOG_ERROR, "bbs plugin_feature_extract error\n");
		}
		
		//该函数是特征码写死的函数，因为referer每个都添加的话太麻烦了
		if(plugin_deal_refererurl(dtts->l4_data, bbs_req)){
	//		log_write(LOG_ERROR, "plugin_deal_refererurl error\n");
		}
		
		if(plugin_data_storage(dtts, bbs_req, thread_num, PLUGIN_BBS_ID)){
//			log_write(LOG_ERROR, "bbs plugin_data_storage() error\n");
		}

        bbs_req->deal_flag = DATA_DEAL;
        plugin_done(bbs_req);
        (dtts->deal_record)->record = NULL;

		return YT_SUCCESSFUL;
		
	}
	
	return YT_FAILED;
}

/*
*函数名称：plugin_deal_account
*函数功能：处理论坛account
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct bbs_request *dst_tmp = (struct bbs_request*)dst;
	
	if(plugin_extract_type_deal(deal_data, dst_tmp->account, sizeof(dst_tmp->account), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->account, sizeof(dst_tmp->account), plugin_fe->encode,  NULL);

RESULT_PRINT("account", dst_tmp->account);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_password
*函数功能：处理论坛password
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct bbs_request *dst_tmp = (struct bbs_request*)dst;
	
	if(plugin_extract_type_deal(deal_data, dst_tmp->password, sizeof(dst_tmp->password), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->password, sizeof(dst_tmp->password), plugin_fe->encode, NULL);

RESULT_PRINT("password", dst_tmp->password);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_title
*函数功能：处理论坛title
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_title(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct bbs_request *dst_tmp = (struct bbs_request*)dst;
	
	if(plugin_extract_type_deal(deal_data, dst_tmp->title, sizeof(dst_tmp->title), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->title, sizeof(dst_tmp->title), plugin_fe->encode, NULL);

RESULT_PRINT("title", dst_tmp->title);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_content
*函数功能：处理论坛内容
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct bbs_request *dst_tmp = (struct bbs_request*)dst;

	if(plugin_extract_type_deal(deal_data, dst_tmp->content, sizeof(dst_tmp->content), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->content, sizeof(dst_tmp->content), plugin_fe->encode, NULL);
	
RESULT_PRINT("content", dst_tmp->content);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_refererurl
*函数功能：处理论坛refererurl
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_refererurl(char *deal_data, void *dst)
{
	char *referer_head = NULL;
	char *referer_tail = NULL;

	struct bbs_request *dst_tmp = (struct bbs_request*)dst;
	referer_head = deal_data;

	do{
		if(!memcmp(referer_head, BBS_REFERER_HEAD, strlen(BBS_REFERER_HEAD))){
			if(plugin_data_extract(referer_head, BBS_REFERER_HEAD, PLUGIN_TAIL, dst_tmp->refererurl, sizeof(dst_tmp->refererurl))){
	//			log_write(LOG_ERROR, "bbs plugin_deal_refererurl error\n");
				return YT_FAILED;
			}
			return YT_SUCCESSFUL;
		}

		referer_tail = strstr(referer_head, PLUGIN_TAIL);
		if(referer_tail){
			referer_head = referer_tail + strlen(PLUGIN_TAIL);
		}
		else{
			return YT_FAILED;
		}

	}while(memcmp(referer_head, PLUGIN_TAIL, strlen(PLUGIN_TAIL)));
	
RESULT_PRINT("refererurl", dst_tmp->refererurl);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_subbbs
*函数功能：处理论坛subbs
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_subbbs(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct bbs_request *dst_tmp = (struct bbs_request*)dst;

	if(plugin_extract_type_deal(deal_data, dst_tmp->subbbs, sizeof(dst_tmp->subbbs), plugin_fe)){
		return YT_FAILED;
	}

	convert_encodes(dst_tmp->subbbs, sizeof(dst_tmp->subbbs), plugin_fe->encode, NULL);

RESULT_PRINT("subbbs", dst_tmp->subbbs);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：bbs_req：数据结构体 thread_num：线程号 plugin_id:插件id
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct bbs_request *bbs_req, const int thread_num, const int plugin_id)
{
	struct plugin_element element_tmp;
	memset(&element_tmp, 0, sizeof(struct plugin_element));

	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);
	//	vlan_id

	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", BBS_PRIFIX, bbs_suffix);

	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}   


	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%s','%s','%s','%d','%s','%s','%s','%d','%s','%s %s','%s','%s','%s','%d','%s','%s','%s','%d','%s');", 
			table_name, 
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,fsubbbsname,faccount,fpassword,faction,ftitle,fcontent,fattachmentname,fattachmentsize,fattachmentlocalpath,fregtime,fsourceip,fsourceipv6,fsourcemac,fsourceport,fdestinationip,fdestinationipv6,fdestinationmac,fdestinationport,frefererurl",
			place_info[0],
			place_info[1],
			bbs_req->bbs_feature->fprotocolid,
			bbs_req->bbs_feature->fprotocolsubid,
			bbs_req->bbs_feature->fprotocolname,
			bbs_req->subbbs,
			bbs_req->account,
			bbs_req->password,
			bbs_req->bbs_feature->action,
			bbs_req->title,
			bbs_req->content,
			bbs_req->attachment_name,
			bbs_req->attachment_size,
			bbs_req->attachment_path,
			bbs_req->date,
			bbs_req->time,
			element_tmp.sip,
			element_tmp.sip6,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dip6,
			element_tmp.dmac,
			element_tmp.dport,
			bbs_req->refererurl
				);


	if(storage_unit_storage_data(storage_buf, thread_num)){
//		log_write(LOG_ERROR, "bbs storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：bbs_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(void *bbs_dst)
{	
	if(bbs_dst)
	{
		mempool_free(bbs_dst);
	}
}
