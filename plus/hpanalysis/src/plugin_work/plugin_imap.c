/***********************************************************
*文件名：plugin_imap.c
*创建人：爱超
*日  期：2014年08月05日
*描  述：imap插件初始化及数据处理
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_public.h"
#include "plugin_imap.h"

//全局数组用于匹配特征码字段
static char *imap_field_name[MAIL_FEATURE_FIELD_NUM] = {PLUGIN_ACCOUNT, PLUGIN_PASSWORD, PLUGIN_FROMACCOUNT, PLUGIN_TOACCOUNT, 
	PLUGIN_CCACCOUNT, PLUGIN_TITLE, PLUGIN_CONTENT, PLUGIN_ATTACHMENT, PLUGIN_MAIL_SERVER, PLUGIN_QUIT}; 

//特征码链表头
static struct plugin_feature imap_feature_head;

//文件存放路径
static char *imap_plugin_path = NULL;

//邮件存储累加
static unsigned int imap_storage_num = 0;

//表名前缀
static char imap_suffix[COMMON_LENGTH_32] = {0};

/*
*函数名称：plugin_init
*函数功能：初始化imap插件
*输入参数：无
*输出参数：0 成功 -1错误
 */
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_imap 模块初始化\n");
	struct plugin_info ppi;
	struct yt_config *config_tmp = NULL;
	
	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "imap base_get_configuration() error\n");
		return YT_FAILED;
	}
	
	imap_plugin_path = config_tmp->plugin_storage_path;
	
	//初始化特征码链表头
	INIT_LIST_HEAD(&(imap_feature_head.list_node));

	//获取特征码
	if(feature_manager_get_feature(&imap_feature_head, IMAP_GROUP_ID)){
		log_write(LOG_ERROR, "imap feature_manager_get_feature() error\n");
		return YT_FAILED;
	}

	//特征码初始化函数
	plugin_feature_init();	

	//插件注册
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = IMAP_GROUP_ID;
	ppi.port = IMAP_PORT;
	ppi.func_plugin_deal = plugin_deal;

	if(plugin_manager_register(&ppi)){
		log_write(LOG_ERROR, "imap plugin_manager_register() error\n");
		return YT_FAILED;
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
static void plugin_feature_init(void)
{
	int i = 0;
	
	struct list_head *list_head_tmp = NULL;
	struct list_head *list_head_extract_tmp = NULL;
	struct plugin_feature *imap_feature_tmp = NULL;
	struct plugin_feature_extract *imap_fe = NULL;
	
	//循环遍历整个特征码
	list_for_each(list_head_tmp, &(imap_feature_head.list_node)){
		imap_feature_tmp = (struct plugin_feature*)list_head_tmp;
		//循环遍历整个截取特征码
		list_for_each(list_head_extract_tmp, &(imap_feature_tmp->pextract_list_node)){
			imap_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
			//依次匹配字段名
			for(i = 0;i < MAIL_FEATURE_FIELD_NUM; i++){
				if(!memcmp(imap_fe->field_name, imap_field_name[i], strlen(imap_field_name[i]))){
					//匹配到字段名之后设置回调函数	
					switch (i){	
						case PLUGIN_ACCOUNT_NUM:
							imap_fe->excb = plugin_deal_account;
							break;
					
						case PLUGIN_PASSWORD_NUM:
							imap_fe->excb = plugin_deal_password;
							break;
			
						case PLUGIN_TITLE_NUM:
							imap_fe->excb = plugin_deal_title;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
							break;

						case PLUGIN_FROMACCOUNT_NUM: 
							imap_fe->excb = plugin_deal_from;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
							break;
				
						case PLUGIN_TOACCOUNT_NUM:
							imap_fe->excb = plugin_deal_to;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
							break;
			
						case PLUGIN_CCACCOUNT_NUM:
							imap_fe->excb = plugin_deal_cc;
						//	imap_fe->server_or_client = SERVER_FEATURE;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
							break;
		
						case PLUGIN_CONTENT_NUM:
							imap_fe->excb = plugin_deal_content;
						//	imap_fe->server_or_client = SERVER_FEATURE;
							break;
			
						case PLUGIN_ATTACHMENT_NUM:
							imap_fe->excb = plugin_deal_attachment;
						//	imap_fe->server_or_client = SERVER_FEATURE;
							break;
				
						case PLUGIN_MAIL_SERVER_NUM:
							imap_fe->excb = plugin_deal_mail_server;
						//	imap_fe->server_or_client = SERVER_FEATURE;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
							break;
					
						case PLUGIN_QUIT_NUM:
							imap_fe->excb = plugin_deal_quit;
						//	imap_fe->server_or_client = SERVER_FEATURE;
							imap_fe->offset_or_tail = PLUGIN_TAIL;
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
*函数名称：plugin_deal
*函数功能：处理数据包
*输入参数：dtts：待处理的数据结构体
		   thread_num：处理的线程号，用于存储模块
*输出参数：无
*返回值：  0成功 -1失败
*/
static int plugin_deal(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts){
		return YT_FAILED;
	}

	struct imap_record *imap_dst = NULL;
	
	imap_dst = (struct imap_record*)((dtts->deal_record)->record);
	
	//连接关闭或超时
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){

		//存在最后没有退出且没有下封邮件的情况
		if(imap_dst){
			if(imap_dst->mail_file){
				fclose(imap_dst->mail_file);
				imap_dst->mail_file = NULL;
			}
			
			if(imap_dst->eml_size > 0){
				if(plugin_data_storage(dtts, imap_dst, thread_num, PLUGIN_MAIL_ID)){
//					log_write(LOG_ERROR, "imap plugin_data_storage error\n");
				}

			}
		}
		
		plugin_done(imap_dst);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;
		
		return YT_SUCCESSFUL;
	}
	
	//客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		if(plugin_deal_client(dtts, thread_num)){
			return YT_FAILED;
		}	
	} else if((dtts->pkt_info).client_or_server == FROM_SERVER_DATA){
	//服务端数据
		if(plugin_deal_server(dtts, imap_dst, thread_num)){
			return YT_FAILED;
		}
	}
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*调用函数：匹配函数、截取函数
*输入参数：dtts：待处理数据 thread_num: 线程号 
*输出参数：dtts->deal_record->record: 存放截取数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, const int thread_num)
{
	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}

	struct imap_record *record = NULL;
	struct plugin_feature *marked_feature = NULL;
	time_t imap_time;
	struct tm *imap_time_now;
	
	record = (struct imap_record*)(dtts->deal_record->record);

	//只匹配一个进行处理
	marked_feature = plugin_feature_mark(dtts->l4_data, dtts->l4_data_size, &imap_feature_head);
	if(marked_feature){
        if (record) {
            plugin_done(record);
            dtts->deal_record->record = NULL;
        }
        record = (struct imap_record*)mempool_malloc(sizeof(struct imap_record));
        if(!record){
            log_write(LOG_ERROR, "imap_record malloc() error\n");
            return YT_FAILED;
        }
        dtts->deal_record->record = record;
		record->imap_feature = marked_feature;

	    //存储imap数据包方向标志位
        record->client_or_server = (dtts->pkt_info).client_or_server;

		//先获得下时间
		time(&imap_time);
		imap_time_now = localtime(&imap_time);	
		sprintf(record->date, "%04d-%02d-%02d", imap_time_now->tm_year + 1900, 
				imap_time_now->tm_mon + 1, imap_time_now->tm_mday);

		sprintf(imap_suffix, "%04d%02d%02d", imap_time_now->tm_year + 1900,
				imap_time_now->tm_mon + 1, imap_time_now->tm_mday);

		sprintf(record->time, "%02d:%02d:%02d", imap_time_now->tm_hour, 
				imap_time_now->tm_min, imap_time_now->tm_sec);
		

		if(plugin_feature_extract(dtts->l4_data, dtts->l4_data_size, &(marked_feature->pextract_list_node), record)){
            log_write(LOG_ERROR, "imap plugin_feature_extract() error\n");
		}
		
		if(plugin_data_storage(dtts, record, thread_num, PLUGIN_MAIL_ID)){
            log_write(LOG_ERROR, "imap plugin_data_storage() error\n");
		}

		return YT_SUCCESSFUL;
	}
	
	return YT_FAILED;
}

/*
*函数名称：plugin_deal_server
*函数功能：处理服务端数据
*调用函数：匹配函数、截取函数
*输入参数：dtts：待处理数据 thread_num:线程号
*输出参数：imap_dst：存储处理后数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_server(const struct data_transit *dtts,  struct imap_record *imap_dst, const int thread_num)
{
	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}

	struct imap_record *record = NULL;
	struct plugin_feature *marked_feature = NULL;
	time_t imap_time;
	struct tm *imap_time_now;
	
	record = (struct imap_record*)(dtts->deal_record->record);

	//只匹配一个进行处理
	marked_feature = plugin_feature_mark(dtts->l4_data, dtts->l4_data_size, &imap_feature_head);
	if(marked_feature){
        if (!record) {
            record = (struct imap_record*)mempool_malloc(sizeof(struct imap_record));
            if(!record){
                log_write(LOG_ERROR, "imap_record malloc() error\n");
                return YT_FAILED;
            }
            dtts->deal_record->record = record;
        }

		record->imap_feature = marked_feature;

	    //存储imap数据包方向标志位
        record->client_or_server = (dtts->pkt_info).client_or_server;

		//先获得下时间
		time(&imap_time);
		imap_time_now = localtime(&imap_time);	
		sprintf(record->date, "%04d-%02d-%02d", imap_time_now->tm_year + 1900, 
				imap_time_now->tm_mon + 1, imap_time_now->tm_mday);

		sprintf(imap_suffix, "%04d%02d%02d", imap_time_now->tm_year + 1900,
				imap_time_now->tm_mon + 1, imap_time_now->tm_mday);

		sprintf(record->time, "%02d:%02d:%02d", imap_time_now->tm_hour, 
				imap_time_now->tm_min, imap_time_now->tm_sec);
		

		if(plugin_feature_extract(dtts->l4_data, dtts->l4_data_size, &(marked_feature->pextract_list_node), record)){
            log_write(LOG_ERROR, "imap plugin_feature_extract() error\n");
		}
		
		if(plugin_data_storage(dtts, record, thread_num, PLUGIN_MAIL_ID)){
            log_write(LOG_ERROR, "imap plugin_data_storage() error\n");
		}

		return YT_SUCCESSFUL;
	}
}
	
/*
*函数名称：plugin_storage_mail
*函数功能：存储邮件
*输入函数：data：待存储邮件数据 data_len：数据长度 thread_num:线程号用于拼接邮件名
*输出参数：imap_dst:存储结构体
*返回值：  0表示成功 -1表示失败
*/
static int plugin_storage_mail(char *data, int data_len, struct imap_record *imap_dst, const int thread_num)
{

	//imap_dst->eml_real_size < 0 代表截取长度失败
	if(!(imap_dst->eml_real_size < 0) && ((imap_dst->eml_size == imap_dst->eml_real_size) || (imap_dst->eml_size > imap_dst->eml_real_size))){
		imap_dst->retr_flag = 0;
		//这个reture把邮件最后的.过滤掉了，过不过一样。
		return YT_FAILED;
	}
char path_temp[COMMON_LENGTH_64] = {0};

	//如果邮件大小为0代表邮件第一段 要拼接邮件存储路径
	if(!(imap_dst->mail_file)){
		sprintf(path_temp, "%s/%s", imap_plugin_path, imap_dst->date);
		if(access(path_temp, 0)){
			if(mkdir(path_temp, 755)){
				log_write(LOG_ERROR, "imap plugin_storage_mail() mkdir() error\n");
				return YT_FAILED;
			}
			//重置累加数		
//			imap_storage_num = 0;
		}
		
		//拼接文件名，打开文件句柄，并将句柄保存
		sprintf(imap_dst->eml_localpath, "%s/%s-imap-%d-%d.eml", path_temp, imap_dst->time, thread_num, imap_storage_num);
		
		imap_storage_num++;

		imap_dst->mail_file = fopen(imap_dst->eml_localpath, "a+");
	
		if(!(imap_dst->mail_file)){
			log_write(LOG_ERROR, "imap plugin_storage_mail() fopen error\n");
			return YT_FAILED;
		}
	}
	
	imap_dst->eml_size += data_len;

	if(plugin_write_file("a+", data, data_len, imap_dst->mail_file)){
		log_write(LOG_ERROR, "imap plugin_write_file error\n");
		return YT_FAILED;
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_account
*函数功能：处理邮件插件
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体  
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_account(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).account_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	//account 啥也不用处理直接截取存放
	if(plugin_extract_type_deal(deal_data, dst_tmp->account, sizeof(dst_tmp->account), plugin_fe)){
//		log_write(LOG_ERROR, "imap account deal error()\n");
		return YT_FAILED;
	}

	(dst_tmp->deal_flag).account_deal_flag = DATA_DEAL;
RESULT_PRINT("account", dst_tmp->account);	

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_password
*函数功能：处理邮件插件
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_password(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).password_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	//password 啥也不用处理直接截取存放
	if(plugin_extract_type_deal(deal_data, dst_tmp->password, sizeof(dst_tmp->password), plugin_fe)){
//		log_write(LOG_ERROR, "imap password deal error()\n");
		return YT_FAILED;
	}

RESULT_PRINT("pass", dst_tmp->password);	
	(dst_tmp->deal_flag).password_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_quit
*函数功能：处理退出
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_quit(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	//quit只处理一个不存在一个包需要多种特征码截取的情况，无需判断
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	(dst_tmp->deal_flag).quit_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_from
*函数功能：处理邮件发送
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体 
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_from(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).from_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	char from_tmp[COMMON_LENGTH_64 + 1] = {0};
	
	//截取from
	if(plugin_extract_type_deal(deal_data, from_tmp, COMMON_LENGTH_64, plugin_fe)){
		return YT_FAILED;		
	}
	//去除<>
	if(plugin_address_deal(from_tmp, dst_tmp->from, sizeof(dst_tmp->from))){
		log_write(LOG_ERROR, "plugin_address_deal error()\n");
		return YT_FAILED;	
	}

RESULT_PRINT("from", dst_tmp->from);

	(dst_tmp->deal_flag).from_deal_flag = DATA_DEAL;
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_to
*函数功能：处理邮件插件
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_to(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;

	if((dst_tmp->deal_flag).to_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	char to_tmp[COMMON_LENGTH_1024 + 1] = {0};
	
	//截取to
	if(plugin_extract_type_deal(deal_data, to_tmp, COMMON_LENGTH_1024, plugin_fe)){
		return YT_FAILED;		
	}
	
	//去除<>
	if(plugin_address_deal(to_tmp, dst_tmp->to, sizeof(dst_tmp->to))){
		log_write(LOG_ERROR, "plugin_address_deal error()\n");
		return YT_FAILED;	
	}

RESULT_PRINT("to", dst_tmp->to);

	(dst_tmp->deal_flag).to_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_cc
*函数功能：处理邮件插件
*输入参数：deal_data：待处理数据  plugin_fe:对应的特征码结构体  
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_cc(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).cc_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	char cc_tmp[COMMON_LENGTH_1024 + 1] = {0};

	//截取cc
	if(plugin_extract_type_deal(deal_data, cc_tmp, COMMON_LENGTH_1024, plugin_fe)){
		return YT_FAILED;		
	}
	
	//去除<>
	if(plugin_address_deal(cc_tmp, dst_tmp->cc, sizeof(dst_tmp->cc))){
		log_write(LOG_ERROR, "plugin_address_deal error()\n");
		return YT_FAILED;	
	}
	
RESULT_PRINT("cc", dst_tmp->cc);

	(dst_tmp->deal_flag).cc_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_title
*函数功能：处理邮件标题
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码 
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_title(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).title_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

    char *p;
	char title_tmp[TITLE_LENGTH + 1] = {0};
	char title_decode[TITLE_LENGTH + 1] = {0};

	//title可能需要转码
	if(plugin_extract_type_deal(deal_data, title_tmp, sizeof(title_tmp), plugin_fe)){
		return YT_FAILED;
	}

	//Subject: =?gb18030?B?cG9wM9b3zOI=?= 
	//Subject: =?UTF-8?B?aOmjjuWTh+WYjuazleWwlOWTh+WThw==?=
	//这里只考虑两种编码方式都并且没有考虑"=?gb2312?B?yerS+M3yufpf0MK5ybaovNtfMzAwMTU4KE7V8barKV/Cpsql7qOjrMLe+YJf?=  =?gb2312?B?o6qjql/UpLzGyc/K0MrXyNW5ybzbMzktNDTUqqGj?=";这种情况
	
	if(p = strcasestr(title_tmp, DECODE_STRING_GB2312)){
		memcpy(title_decode, p + strlen(DECODE_STRING_GB2312), 
				strlen(title_tmp) - strlen(DECODE_STRING_GB2312));
		base64_decode(title_decode, dst_tmp->title);
		convert_gb18030_to_utf8(dst_tmp->title, sizeof(dst_tmp->title));
	}
	else if(p = strcasestr(title_tmp, DECODE_STRING_GBK)){
		memcpy(title_decode, p + strlen(DECODE_STRING_GBK), 
				strlen(title_tmp) - strlen(DECODE_STRING_GBK));
		base64_decode(title_decode, dst_tmp->title);
		convert_gb18030_to_utf8(dst_tmp->title, sizeof(dst_tmp->title));
	}
	else if(p = strcasestr(title_tmp, DECODE_STRING_UTF_8)){
		memcpy(title_decode, p + strlen(DECODE_STRING_UTF_8), 
				strlen(title_tmp) - strlen(DECODE_STRING_UTF_8));
		base64_decode(title_decode, dst_tmp->title);
	}
	else{
		memcpy(dst_tmp->title, title_tmp, sizeof(title_tmp));
	}

RESULT_PRINT("title", dst_tmp->title);

	(dst_tmp->deal_flag).title_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_mail_server
*函数功能：处理邮件mail_server
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_mail_server(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct imap_record *dst_tmp = (struct imap_record*)dst;

	if((dst_tmp->deal_flag).server_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	//mail-server啥都不用处理直接截取
	if(plugin_extract_type_deal(deal_data, dst_tmp->mail_server, sizeof(dst_tmp->mail_server), plugin_fe)){
		return YT_SUCCESSFUL;
	}

	(dst_tmp->deal_flag).server_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_real_deal_content
*函数功能：处理邮件主题内容
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_real_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{

	struct imap_record *dst_tmp = (struct imap_record*)dst;
	char encode_tmp[COMMON_LENGTH_1024 + 1] = {0};
	
	char *content_head = NULL;

	int base64_flag = 0;
	int gb_flag = 0;
	
	//先将后边的添加结束标志，防止接下来找编码无限找下去
	//这样即便From等先截截取的时候也能防止无限找下去，附件查找头会移动到内容后
	content_head = strstr(deal_data, PLUGIN_TAIL_TAIL);
	if(content_head){
		memcpy(encode_tmp, deal_data, MIN(COMMON_LENGTH_1024, content_head - deal_data));
	//	*content_head = '\0';
		content_head = content_head + strlen(PLUGIN_TAIL_TAIL);
		
		(dst_tmp->deal_flag).eml_content_head = content_head;
		*((dst_tmp->deal_flag).eml_content_tail) = '\0';
		

		//接下来有附件要搜索附件了，把指针往下指
		(dst_tmp->deal_flag).attachment_head = (dst_tmp->deal_flag).eml_content_tail + strlen(POP3_TAIL);
	}
	else{
		return YT_FAILED;
	}

	//是base64先转码 UTF8的不用转
	if(strcasestr(encode_tmp, "Content-Transfer-Encoding: base64")){
		base64_flag = 1;
	}
	
	//是gb2312则转码
	if(LIKELY(strcasestr(encode_tmp, "gb2312")) || LIKELY(strcasestr(encode_tmp, "gb18030"))
			|| strcasestr(encode_tmp, "gbk")){
		gb_flag = 1;
	}

	//可能存在(dst_tmp->deal_flag).eml_content_head长度大于dst_tmp->eml_content,先不考虑
	if(base64_flag){
		base64_decode((dst_tmp->deal_flag).eml_content_head, dst_tmp->eml_content);
	}
	
	if(gb_flag){
		convert_gb18030_to_utf8(dst_tmp->eml_content, sizeof(dst_tmp->eml_content));
	}

	//将尾巴改回来不然strncat拼接不上了
	*((dst_tmp->deal_flag).eml_content_tail) = '\r';

	//没编码的话就直接拷贝到目的地址，一般都会编码
	if(UNLIKELY(!base64_flag && !gb_flag)){
		memcpy(dst_tmp->eml_content, (dst_tmp->deal_flag).eml_content_head, 
				MIN(sizeof(dst_tmp->eml_content), (dst_tmp->deal_flag).eml_content_tail - 
				(dst_tmp->deal_flag).eml_content_head));
	}
	
RESULT_PRINT("content", dst_tmp->eml_content);
	
	(dst_tmp->deal_flag).content_deal_flag = DATA_DEAL;

	
	return YT_SUCCESSFUL;	
}

/*
*函数名称：plugin_deal_content
*函数功能：处理邮件主题内容
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;

	if((dst_tmp->deal_flag).content_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	//再找尾的过程中每次都会找内容，递增模式，能不能加一个偏移量。
	if((dst_tmp->deal_flag).content_deal_head == DATA_NO_DEAL){
		(dst_tmp->deal_flag).eml_content_head = strstr(deal_data, plugin_fe->offset_or_head);
		if((dst_tmp->deal_flag).eml_content_head){
			(dst_tmp->deal_flag).content_deal_head = DATA_DEAL;
		}
	}
	if((dst_tmp->deal_flag).content_deal_tail == DATA_NO_DEAL){
		if((dst_tmp->deal_flag).eml_content_head){
			(dst_tmp->deal_flag).eml_content_tail = 
				strstr((dst_tmp->deal_flag).eml_content_head, POP3_TAIL);
		}		
		
		if((dst_tmp->deal_flag).eml_content_tail){
			(dst_tmp->deal_flag).content_deal_tail = DATA_DEAL;				
		}
	}
	
	//如果头尾都找到了那就进行内容的处理
	if((dst_tmp->deal_flag).content_deal_head == DATA_DEAL && 
			(dst_tmp->deal_flag).content_deal_tail == DATA_DEAL){
		if(plugin_real_deal_content((dst_tmp->deal_flag).eml_content_head, dst, plugin_fe)){
			return YT_FAILED;
		}
	}	

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_attachment
*函数功能：处理邮件插件
*输入参数：deal_data：待处理数据 plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_attachment(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe) 
{
PLUGIN_PRINT("++++++++++++++++++++++++++++++++++++++++++\n");
	struct imap_record *dst_tmp = (struct imap_record*)dst;
	
	if((dst_tmp->deal_flag).attachment_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	char attachment_name_tmp[COMMON_LENGTH_256 + 1] = {0};
	char attachment_name_decode[COMMON_LENGTH_256 + 1] = {0};
	char *attachment_head = NULL;
	char *tmp_deal_data = deal_data;
	int i = 0;

	//处理多附件，将多个附件名拼接，以分号间隔
	//若多个附件找到一个之后下一个要遍历附件数据，性能差，看
	//有无改进办法,目前在找到一个附件之后我就不再继续循环了。
//	while(1){
		//查找附件 如果内容处理完了 尽量跳过内容，内容很多的话，效率大大的
		if(LIKELY((dst_tmp->deal_flag).attachment_head)){
			attachment_head = strstr((dst_tmp->deal_flag).attachment_head, 
					plugin_fe->offset_or_head);
		}
		else{
			attachment_head = strstr(tmp_deal_data, plugin_fe->offset_or_head);
		}	

		if(!attachment_head){
			if(0 == i){
				dst_tmp->attachment_flag = 0;
				return YT_FAILED;
			}
			
		//	break;
		}
		
		tmp_deal_data = attachment_head + strlen(plugin_fe->offset_or_head);
		dst_tmp->attachment_flag = 1;
		
		//截取附件名
		if(plugin_data_extract(attachment_head + strlen(plugin_fe->offset_or_head), MAIL_ATTACHMENT_HEAD,
					MAIL_ATTACHMENT_TAIL, attachment_name_tmp, sizeof(attachment_name_tmp))){
			memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
			return YT_FAILED;
		}
		else{
			if(strcasestr(attachment_name_tmp, DECODE_STRING_GBK)){
				memcpy(attachment_name_decode, attachment_name_tmp + strlen(DECODE_STRING_GBK),
						strlen(attachment_name_tmp) - strlen(DECODE_STRING_GBK));
				memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
				base64_decode(attachment_name_decode, attachment_name_tmp);
				convert_gb18030_to_utf8(attachment_name_tmp, 
						sizeof(attachment_name_tmp));
			}
			else if(strcasestr(attachment_name_tmp, DECODE_STRING_UTF_8)){
				memcpy(attachment_name_decode, attachment_name_tmp + strlen(DECODE_STRING_UTF_8),strlen(attachment_name_tmp) - strlen(DECODE_STRING_UTF_8));
				memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
				base64_decode(attachment_name_decode, attachment_name_tmp);
			}
			
			if(i > 0){
				strcat(dst_tmp->eml_attachment_name, ";");
			}
			strcat(dst_tmp->eml_attachment_name, attachment_name_tmp);
			memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
		}
		

		(dst_tmp->deal_flag).attachment_deal_flag = DATA_DEAL;
	
		i++;
//	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*输入参数：imap_dst：数据结构体 thread_num：线程号 plugin_id:插件号
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct imap_record *imap_dst, int thread_num, int plugin_id) 
{
	if(!dtts || !imap_dst){
		return YT_FAILED;
	}

	struct plugin_element element_tmp;
	memset(&element_tmp, 0, sizeof(struct plugin_element));
	
	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);

	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", MAIL_PRIFIX, imap_suffix);

	//比对数据包方向，如果不同，则源、目的IP和源、目的MAC要互转
	if((dtts->pkt_info).client_or_server != imap_dst->client_or_server){
		char tmp[IP_LENGTH + 1] = {0};			//用于交换时的临时存储
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip, element_tmp.dip, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.smac, element_tmp.dmac, tmp);
		memset(tmp, 0, IP_LENGTH+1);
		SWAP_IP_MAC(element_tmp.sip6, element_tmp.dip6, tmp);
	}

	snprintf(storage_buf, STORAGE_BUF_LEN, "insert into %s(%s) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%d','%s','%s','%s','%d','%s','%s %s','%s','%s','%s','%d','%s','%s','%s','%d');", 
			table_name,
			"fplacecode,fplacename,fprotocolid,fprotocolsubid,fprotocolname,fmailserver,faccount,fpassword,ffromaccount,ftoaccount,fccaccount,faction,ftitle,fcontent,fattachmentname,femailsize,femllocalpath,fregtime,fsourceip,fsourceipv6,fsourcemac,fsourceport,fdestinationip,fdestinationipv6,fdestinationmac,fdestinationport",
			place_info[0],
			place_info[1],
			imap_dst->imap_feature->fprotocolid,
			imap_dst->imap_feature->fprotocolsubid,
			imap_dst->imap_feature->fprotocolname,
			imap_dst->mail_server,
			imap_dst->account,
			imap_dst->password,
			imap_dst->from,
			imap_dst->to,
			imap_dst->cc,
			imap_dst->imap_feature->action,
			imap_dst->title,
			imap_dst->eml_content,
			imap_dst->eml_attachment_name,
			imap_dst->eml_size,
			imap_dst->eml_localpath,
			imap_dst->date, 
			imap_dst->time,
			element_tmp.sip,
			element_tmp.sip6,
			element_tmp.smac,
			element_tmp.sport,
			element_tmp.dip,
			element_tmp.dip6,
			element_tmp.dmac,
			element_tmp.dport
				);

	if(storage_unit_storage_data(storage_buf, thread_num)){
		//		log_write(LOG_ERROR, "imap storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
 *函数名称：plugin_done
 *函数功能：插件释放
 *输入参数：imap_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct imap_record *imap_dst)
{	
	if(imap_dst){
		if((imap_dst->deal_flag).imap_data){
			mempool_free((imap_dst->deal_flag).imap_data);
		}
		
		(imap_dst->deal_flag).imap_data = NULL;

		mempool_free(imap_dst);
	}
}

