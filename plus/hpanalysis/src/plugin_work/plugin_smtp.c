/***********************************************************
*文件名：plugin_smtp.c
*创建人：韩涛
*日  期：2013年10月21日
*描  述：smtp插件初始化及数据处理
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE

#include "plugin_public.h"
#include "plugin_smtp.h"

#define	  CONTENT_TYPE_NONE			0
#define   CONTENT_TYPE_HTML			1
#define	  CONTENT_TYPE_PLAIN		2

static char *smtp_field_name[MAIL_FEATURE_FIELD_NUM] = {PLUGIN_ACCOUNT, PLUGIN_PASSWORD, PLUGIN_FROMACCOUNT, PLUGIN_TOACCOUNT, 
	PLUGIN_CCACCOUNT, PLUGIN_TITLE, PLUGIN_CONTENT, PLUGIN_ATTACHMENT, PLUGIN_MAIL_SERVER, PLUGIN_QUIT}; 

//特征码链表头
static struct plugin_feature smtp_feature_head;
//文件存放路径
static char *smtp_plugin_path = NULL;

//邮件存储随机数
static unsigned int smtp_stoarge_num = 0;

//表名前缀
static char mail_suffix[COMMON_LENGTH_32] = {0};

//rsoolve content in html
static int resolve_html_smtp(char *html_str,char *dst_cont,int maxlen);

static char content_type=0;
/*
*函数名称：plugin_init
*函数功能：初始化smtp件
*输入参数：无
*输出参数：0 成功 -1错误
*/
//static int __attribute ((constructor)) plugin_init()
static int __attribute ((constructor)) plugin_init()
{
	INFO_PRINT("Plugin_smtp 模块初始化\n");
	struct plugin_info ppi;
	struct yt_config *config_tmp = NULL;

	//获取插件文件存放路径	
	config_tmp = base_get_configuration();
	if(!config_tmp){
		log_write(LOG_ERROR, "smtp base_get_configuration() error\n");
		return YT_FAILED;
	}

	smtp_plugin_path = config_tmp->plugin_storage_path;
	
	INIT_LIST_HEAD(&(smtp_feature_head.list_node));

	//获取特征码
	if(feature_manager_get_feature(&smtp_feature_head, SMTP_GROUP_ID)){
		log_write(LOG_ERROR, "smtp feature_manager_get_feature() error\n");
		return YT_FAILED;
	}

	//特征码初始化
	plugin_feature_init();

	//插件注册
	memset(&ppi, 0, sizeof(struct plugin_info));
	ppi.plugin_id = SMTP_GROUP_ID;
	ppi.port = SMTP_PORT;
	ppi.func_plugin_deal = plugin_deal;

	if(plugin_manager_register(&ppi)){
		log_write(LOG_ERROR, "smtp plugin_manager_register() error\n");
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
static void plugin_feature_init()
{
	int i = 0;
	
	struct list_head *list_head_tmp = NULL;
	struct list_head *list_head_extract_tmp = NULL;
	struct plugin_feature *smtp_feature_tmp = NULL;
	struct plugin_feature_extract *smtp_fe = NULL;
	
	list_for_each(list_head_tmp, &(smtp_feature_head.list_node)){
		smtp_feature_tmp = (struct plugin_feature*)list_head_tmp;
	
		list_for_each(list_head_extract_tmp, &(smtp_feature_tmp->pextract_list_node)){
			smtp_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
		
			for(i = 0;i < MAIL_FEATURE_FIELD_NUM; i++){
				if(!memcmp(smtp_fe->field_name, smtp_field_name[i], strlen(smtp_field_name[i]))){
					switch (i){	
/*						case PLUGIN_ACCOUNT_NUM:
							smtp_fe->excb = plugin_deal_account;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_PASSWORD_NUM:
							smtp_fe->excb = plugin_deal_password;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
*/							
						case PLUGIN_TITLE_NUM:
							smtp_fe->excb = plugin_deal_title;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_FROMACCOUNT_NUM: 
							smtp_fe->excb = plugin_deal_from;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_TOACCOUNT_NUM:
							smtp_fe->excb = plugin_deal_to;
							smtp_fe->offset_or_head=PLUGIN_SMTP_TO_ACCOUT;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_CCACCOUNT_NUM:
							smtp_fe->excb = plugin_deal_cc;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_CONTENT_NUM:
							smtp_fe->excb = plugin_deal_content;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_ATTACHMENT_NUM:
							smtp_fe->excb = plugin_deal_attachment;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_MAIL_SERVER_NUM:
							smtp_fe->excb = plugin_deal_mail_server;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
							break;
						case PLUGIN_QUIT_NUM:
							smtp_fe->excb = plugin_deal_quit;
							smtp_fe->offset_or_tail = PLUGIN_TAIL;
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
*返回值：0成功 -1失败
*/
static int plugin_deal(const struct data_transit *dtts, const int thread_num)
{

	if(!dtts){
		return YT_FAILED;
	}
	
	struct mail_smtp *smtp_dst = NULL;
	
	smtp_dst = ((dtts->deal_record)->record);
	
	//连接关闭或超时
	if(dtts->session_status == SESSION_STATUS_CLOSE || 
			dtts->session_status == SESSION_STATUS_TIMEOUT){	
		if(smtp_dst){		
			if(smtp_dst->mail_file){	
				fclose(smtp_dst->mail_file);
				smtp_dst->mail_file = NULL;
			}
		
			if(smtp_dst->eml_size > 0){
				//存储邮件
				if(plugin_data_storage(dtts, smtp_dst, thread_num, PLUGIN_MAIL_ID)){
			//		log_write(LOG_ERROR, "smtp plugin_data_storage() error\n");
			//			return YT_FAILED;
				}
			}
		}
	
		plugin_done(smtp_dst);
		(dtts->deal_record)->record = NULL;
		(dtts->deal_record)->record_destory_flag = RECORD_CAN_DESTORY;
		
		return YT_SUCCESSFUL;
	}

	//服务端数据，分配空间
	if((dtts->pkt_info).client_or_server == FROM_SERVER_DATA){
		if(plugin_deal_server(dtts, thread_num)){
			return YT_FAILED;
		}	
	}
	
	smtp_dst = ((dtts->deal_record)->record);

	//客户端数据
	if((dtts->pkt_info).client_or_server == FROM_CLIENT_DATA){
		if(plugin_deal_client(dtts, smtp_dst, thread_num)){
			return YT_FAILED;
		}	
	}
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_server
*函数功能：处理客户端数据
*输入参数：dtts:待处理数据包  thread_num:线程号 
*输出参数：无
*返回值：  0成功 -1失败
*/

static int plugin_deal_server(const struct data_transit *dtts, const int thread_num)
{
	struct plugin_feature *marked_feature = NULL;
	time_t smtp_time;
	struct tm *smtp_time_now;
	struct mail_smtp *smtp_dst = NULL;

	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size)){
		return YT_FAILED;
	}

	//匹配特征码
	marked_feature = plugin_feature_mark(dtts->l4_data, dtts->l4_data_size, &smtp_feature_head);
	if(marked_feature){
		smtp_dst = (dtts->deal_record)->record;
	
		if(!smtp_dst){
			smtp_dst = (struct mail_smtp *)mempool_malloc(sizeof(struct mail_smtp));
		
			if(!smtp_dst){
				log_write(LOG_ERROR, "smtp smtp_dst malloc() error\n");
				return YT_FAILED;
			}
			
			(dtts->deal_record)->record = smtp_dst;

			smtp_dst->mail_file = NULL;
			(smtp_dst->deal_flag).smtp_data = NULL;	
		}
		
		
		//退出
		if(!memcmp(dtts->l4_data, SMTP_QUIT, strlen(SMTP_QUIT))){
				
			char account_tmp[ACCOUNT_LENGTH + 1] ={0};
			char password_tmp[PASSWORD_LENGTH + 1] = {0};

			//如果是邮件之后的第一个250，则入库
			if(smtp_dst->data_flag){	
				smtp_dst->data_flag = 0;
			
				if(smtp_dst->mail_file){	
					fclose(smtp_dst->mail_file);
					smtp_dst->mail_file = NULL;
				}

				if(smtp_dst->eml_size > 0){
					//存储邮件
					if(plugin_data_storage(dtts, smtp_dst, thread_num, PLUGIN_MAIL_ID)){
					//	log_write(LOG_ERROR, "smtp plugin_data_storage() error\n");
						return YT_FAILED;
					}
				}
			
				//被压内存吓怕了，所以采用这种低效率的方式试试
				memcpy(account_tmp, smtp_dst->account, ACCOUNT_LENGTH);
				memcpy(password_tmp, smtp_dst->password, PASSWORD_LENGTH);
			
				if((smtp_dst->deal_flag).smtp_data){
					mempool_free((smtp_dst->deal_flag).smtp_data);
				}	
				
				memset(smtp_dst, 0, sizeof(struct mail_smtp));
				
				(smtp_dst->deal_flag).smtp_data = NULL;
				smtp_dst->mail_file = NULL;

				memcpy(smtp_dst->account, account_tmp, ACCOUNT_LENGTH);
				memcpy(smtp_dst->password, password_tmp, PASSWORD_LENGTH);
			
			
				//获取时间
				time(&smtp_time);
				smtp_time_now = localtime(&smtp_time);	
				sprintf(smtp_dst->date, "%04d-%02d-%02d", smtp_time_now->tm_year + 1900, 
						smtp_time_now->tm_mon + 1, smtp_time_now->tm_mday);

				sprintf(mail_suffix, "%04d%02d%02d", smtp_time_now->tm_year + 1900,
						smtp_time_now->tm_mon + 1, smtp_time_now->tm_mday);

				sprintf(smtp_dst->time, "%02d:%02d:%02d", smtp_time_now->tm_hour, 
						smtp_time_now->tm_min, smtp_time_now->tm_sec);
				
				smtp_dst->smtp_feature = marked_feature;
			
				//存储退出
				if(plugin_data_storage(dtts, smtp_dst, thread_num, PLUGIN_MAIL_ID)){
			//		log_write(LOG_ERROR, "smtp plugin_data_storage() error\n");
					return YT_FAILED;
				}

				plugin_done(smtp_dst);
				(dtts->deal_record)->record = NULL;
			}

			return YT_SUCCESSFUL;
		}
		
		//获取时间
		time(&smtp_time);
		smtp_time_now = localtime(&smtp_time);	
		sprintf(smtp_dst->date, "%04d-%02d-%02d", smtp_time_now->tm_year + 1900, smtp_time_now->tm_mon + 1, smtp_time_now->tm_mday);
		sprintf(mail_suffix, "%04d%02d%02d", smtp_time_now->tm_year + 1900, smtp_time_now->tm_mon + 1, smtp_time_now->tm_mday);
		sprintf(smtp_dst->time, "%02d:%02d:%02d", smtp_time_now->tm_hour, smtp_time_now->tm_min, smtp_time_now->tm_sec); 
		
		smtp_dst->smtp_feature = marked_feature;
	
		//登陆
		if(!memcmp(dtts->l4_data, SMTP_LOGIN, strlen(SMTP_LOGIN))){
			smtp_dst->login_flag = 1;
			
			return YT_SUCCESSFUL;
		}

		
		//新邮件
		if(!memcmp(dtts->l4_data, SMTP_NEW, strlen(SMTP_NEW))){
			smtp_dst->data_flag = 1;

			return YT_SUCCESSFUL;
		}

		return YT_SUCCESSFUL;
	}

	return YT_FAILED;
}

/*
*函数名称：plugin_deal_client
*函数功能：处理客户端数据
*输入参数：dtts:待处理数据包  thread_num:线程号 
*输出参数：smtp_dst：存储处理后数据
*返回值：  0成功 -1失败
*/
static int plugin_deal_client(const struct data_transit *dtts, struct mail_smtp *smtp_dst, const int thread_num)
{

	if(!dtts || !(dtts->l4_data) || !(dtts->l4_data_size) || !smtp_dst){
		return YT_FAILED;
	}
	
	//用户名密码处理
	if(smtp_dst->login_flag){
		
		smtp_dst->login_flag = 0;
		if((smtp_dst->deal_flag).account_deal_flag == DATA_NO_DEAL){		
			plugin_deal_account(dtts->l4_data, smtp_dst);
			return YT_SUCCESSFUL;
		}
	
		if((smtp_dst->deal_flag).password_deal_flag == DATA_NO_DEAL){		
			plugin_deal_password(dtts->l4_data, smtp_dst);
		}
	
	//	if(plugin_feature_extract(dtts->l4_data, dtts->l4_data_size, &((smtp_dst->smtp_feature)->pextract_list_node), smtp_dst)){
	//		log_write(LOG_ERROR, "smtp plugin_feature_extract() error\n");
	//		return YT_FAILED;
	//	}
	//	smtp_dst->can_deal_flag = 1;
		

		if((smtp_dst->deal_flag).account_deal_flag && (smtp_dst->deal_flag).password_deal_flag){
			
			if(plugin_data_storage(dtts, smtp_dst, thread_num, PLUGIN_MAIL_ID)){
	//			log_write(LOG_ERROR, "smtp plugin_data_storage() error\n");
				return YT_FAILED;
			}
		}
	}


//邮件处理
	if(smtp_dst->data_flag){
		
	//	smtp_dst->data_flag = 0;	
		
		//先将邮件存储	
		if(plugin_storage_mail(dtts->l4_data, dtts->l4_data_size, smtp_dst, thread_num)){
	//		log_write(LOG_ERROR, "plugin_storage_mail() error\n");
			return YT_FAILED;
		}
		
		//没找到附件继续堆积，找到附件了就不进行堆积了 直接存储了
		if((smtp_dst->deal_flag).attachment_deal_flag == DATA_NO_DEAL){
		
			if(!((smtp_dst->deal_flag).smtp_data)){
				(smtp_dst->deal_flag).smtp_data = (char*)mempool_malloc(dtts->l4_data_size + 1);

				if(!((smtp_dst->deal_flag).smtp_data)){
					log_write(LOG_ERROR, "smtp plugin_deal_server smtp_data realloc error\n");
					return YT_FAILED;
				}
				
				memcpy((smtp_dst->deal_flag).smtp_data, dtts->l4_data, dtts->l4_data_size);
			}
			else{
				(smtp_dst->deal_flag).smtp_data = (char*)mempool_realloc((smtp_dst->deal_flag).smtp_data, smtp_dst->eml_size + 1);
				if(!((smtp_dst->deal_flag).smtp_data)){
					log_write(LOG_ERROR, "smtp plugin_deal_server smtp_data realloc error\n");
					return YT_FAILED;
				}

				strncat((smtp_dst->deal_flag).smtp_data, dtts->l4_data, dtts->l4_data_size);
				
			}

			//没找到附件继续找，即便真没附件也不是很耗费,内容处理完之后指针会移动
			if(plugin_feature_extract(((smtp_dst->deal_flag).smtp_data), dtts->l4_data_size, &((smtp_dst->smtp_feature)->pextract_list_node), smtp_dst)){
		//		log_write(LOG_ERROR, "smtp plugin_feature_extract() error\n");
				return YT_FAILED;
			}
		}
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_storage_mail
*函数功能：存储邮件
*输入函数：data：待存储邮件数据 data_len：数据长度 thread_num:线程号用于拼接邮件名
*输出参数：smtp_dst:存储结构体
*返回值：  0表示成功 -1表示失败
*/
static int plugin_storage_mail(char *data, int data_len, struct mail_smtp *smtp_dst, const int thread_num)
{
	char path_temp[COMMON_LENGTH_64] = {0};

		//如果邮件大小为0代表邮件第一段 要拼接邮件存储路径
	if(!smtp_dst->mail_file){
		sprintf(path_temp, "%s/%s", smtp_plugin_path, smtp_dst->date);
	
		if(access(path_temp, 0)){
			if(mkdir(path_temp, 755) == YT_FAILED){
				log_write(LOG_ERROR, "smtp plugin_storage_mail() mkdir() error\n");
				return YT_FAILED;
			}
			//重置累加数
			smtp_stoarge_num = 0;	
		}

		sprintf(smtp_dst->eml_localpath, "%s/%s-smtp-%d-%d.eml", path_temp, smtp_dst->time, thread_num, smtp_stoarge_num);

		smtp_stoarge_num++;	

		smtp_dst->mail_file = fopen(smtp_dst->eml_localpath, "a+"); 

		if(!(smtp_dst->mail_file)){
			log_write(LOG_ERROR, "smtp plugin_storage_mail() fopen error\n");
			return YT_FAILED;
		}
	}
	
	smtp_dst->eml_size += data_len;

	if(plugin_write_file("a+", data, data_len, smtp_dst->mail_file)){
		log_write(LOG_ERROR, "smtp plugin_write_file() error\n");
		return YT_FAILED;
	}
	
	return YT_SUCCESSFUL;
}
/*
*函数名称：plugin_deal_account
*函数功能：处理account
*输入参数：deal_data：待处理数据 
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_account(char *deal_data, void *dst)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	char account_tmp[ACCOUNT_LENGTH + 1] = {0};

	if((dst_tmp->deal_flag).account_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	memcpy(account_tmp, deal_data, MIN(sizeof(account_tmp), strlen(deal_data)));

	base64_decode(account_tmp, dst_tmp->account);

	(dst_tmp->deal_flag).account_deal_flag = DATA_DEAL;

RESULT_PRINT("account", dst_tmp->account);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_password
*函数功能：处理password
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码 
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_password(char *deal_data, void *dst)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	char password_tmp[PASSWORD_LENGTH + 1] = {0};
	
	if((dst_tmp->deal_flag).password_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	memcpy(password_tmp, deal_data, MIN(sizeof(password_tmp), strlen(deal_data)));
	
	base64_decode(password_tmp, dst_tmp->password);

	(dst_tmp->deal_flag).password_deal_flag = DATA_DEAL;

RESULT_PRINT("password", dst_tmp->password);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_from
*函数功能：处理邮件from
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_from(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	if((dst_tmp->deal_flag).from_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	char from_tmp[COMMON_LENGTH_64 + 1] = {0};
	
	if(plugin_extract_type_deal(deal_data, from_tmp, COMMON_LENGTH_64, plugin_fe)){
		return YT_FAILED;
	}
	
	if(plugin_address_deal(from_tmp, dst_tmp->from, sizeof(dst_tmp->from))){
		log_write(LOG_ERROR, "smtp plugin_address_deal() error\n");
		return YT_SUCCESSFUL;
	}

	(dst_tmp->deal_flag).from_deal_flag = DATA_DEAL;
	
RESULT_PRINT("from", dst_tmp->from);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_to
*函数功能：处理邮件to
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_to(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	if((dst_tmp->deal_flag).to_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	char to_tmp[COMMON_LENGTH_1024 + 1] = {0};
	
	if(plugin_extract_type_deal(deal_data, to_tmp, COMMON_LENGTH_1024, plugin_fe)){
		return YT_FAILED;
	}
	
	if(plugin_address_deal(to_tmp, dst_tmp->to, sizeof(dst_tmp->to))){
		log_write(LOG_ERROR, "smtp plugin_address_deal() error\n");
		return YT_SUCCESSFUL;
	}

	(dst_tmp->deal_flag).to_deal_flag = DATA_DEAL;

RESULT_PRINT("to", dst_tmp->to);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_cc
*函数功能：处理邮件cc
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_cc(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	if((dst_tmp->deal_flag).cc_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	
	char cc_tmp[COMMON_LENGTH_1024 + 1] = {0};

	if(plugin_extract_type_deal(deal_data, cc_tmp, COMMON_LENGTH_1024, plugin_fe)){
		return YT_FAILED;
	}
	
	if(plugin_address_deal(cc_tmp, dst_tmp->cc, sizeof(dst_tmp->cc))){
		log_write(LOG_ERROR, "smtp plugin_address_deal() error\n");
		return YT_SUCCESSFUL;
	}
	
	(dst_tmp->deal_flag).cc_deal_flag = DATA_DEAL;

RESULT_PRINT("cc", dst_tmp->cc);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_title
*函数功能：处理邮件title
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_title(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	if((dst_tmp->deal_flag).title_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	char title_tmp[TITLE_LENGTH + 1] = {0};
	char title_decode[TITLE_LENGTH + 1] = {0};

	//title可能需要转码
	if(plugin_extract_type_deal(deal_data, title_tmp, TITLE_LENGTH, plugin_fe)){
		return YT_SUCCESSFUL;
	}

	//Subject: =?gb18030?B?cG9wM9b3zOI=?= 
	//Subject: =?UTF-8?B?aOmjjuWTh+WYjuazleWwlOWTh+WThw==?=
	//这里只考虑两种编码方式都并且没有考虑"=?gb2312?B?yerS+M3yufpf0MK5ybaovNtfMzAwMTU4KE7V8barKV/Cpsql7qOjrMLe+YJf?=  =?gb2312?B?o6qjql/UpLzGyc/K0MrXyNW5ybzbMzktNDTUqqGj?=";这种情况

	if(strcasestr(title_tmp, DECODE_STRING_GB2312)){
		memcpy(title_decode, title_tmp + strlen(DECODE_STRING_GB2312), 
				strlen(title_tmp) - strlen(DECODE_STRING_GB2312));
		base64_decode(title_decode, dst_tmp->title);
		convert_gb18030_to_utf8(dst_tmp->title, sizeof(dst_tmp->title));
	}
	else if(strcasestr(title_tmp, DECODE_STRING_GBK)){
		memcpy(title_decode, title_tmp + strlen(DECODE_STRING_GBK), 
				strlen(title_tmp) - strlen(DECODE_STRING_GBK));
		base64_decode(title_decode, dst_tmp->title);
		convert_gb18030_to_utf8(dst_tmp->title, sizeof(dst_tmp->title));
	}
	else if(strcasestr(title_tmp, DECODE_STRING_UTF_8)){
		memcpy(title_decode, title_tmp + strlen(DECODE_STRING_UTF_8), 
				strlen(title_tmp) - strlen(DECODE_STRING_UTF_8));
		base64_decode(title_decode, dst_tmp->title);
	}
	else{
		memcpy(dst_tmp->title, title_tmp, sizeof(title_tmp));
	}

	(dst_tmp->deal_flag).title_deal_flag = DATA_DEAL;

RESULT_PRINT("title", dst_tmp->title);

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
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;

	if((dst_tmp->deal_flag).server_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	//mail-server啥都不用处理直接截取
	if(plugin_extract_type_deal(deal_data, dst_tmp->mail_server, sizeof(dst_tmp->mail_server), plugin_fe)){
		return YT_FAILED;
	}
	
	(dst_tmp->deal_flag).server_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_quit
*函数功能：处理退出
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：  0 成功 -1 失败
*/
static int plugin_deal_quit(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;

	if((dst_tmp->deal_flag).quit_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}

	//以后可能有其他处理

	(dst_tmp->deal_flag).quit_deal_flag = DATA_DEAL;

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_real_deal_content_plain
*函数功能：处理邮件主题内容
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_real_deal_content_plain(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	char *content_head = NULL;

	int base64_flag = 0;
	int gb_flag = 0;
	
	//先将后边的添加结束标志，防止接下来找编码无限找下去
	content_head = strstr(deal_data, PLUGIN_TAIL_TAIL);
	if(content_head){
		//*content_head = '\0';
		content_head = content_head + strlen(PLUGIN_TAIL_TAIL);
		
		(dst_tmp->deal_flag).eml_content_head = content_head;
		*((dst_tmp->deal_flag).eml_content_tail) = '\0';
		//接下来有附件要搜索附件了，把指针往下指
		(dst_tmp->deal_flag).attachment_head = (dst_tmp->deal_flag).eml_content_tail + strlen(SMTP_TAIL);
	}

	//是base64先转码 UTF8的不用转
	if(strcasestr(deal_data, "Content-Transfer-Encoding: base64")){
		base64_flag = 1;
	}
	
	//是gb2312则转码
	if(LIKELY(strcasestr(deal_data, "gb2312")) || LIKELY(strcasestr(deal_data, "gb18030"))
			|| strcasestr(deal_data, "gbk")){
		gb_flag = 1;
	}

	//可能存在(dst_tmp->deal_flag).eml_content_head长度大于dst_tmp->eml_content,先不考虑
	if(base64_flag){
		base64_decode((dst_tmp->deal_flag).eml_content_head, dst_tmp->eml_content);
	}
	
	if(gb_flag){
		convert_gb18030_to_utf8(dst_tmp->eml_content, sizeof(dst_tmp->eml_content));
	}

	//把尾巴改回来不然strncat拼接不上
	*((dst_tmp->deal_flag).eml_content_tail) = '\r';

	//没编码的话就直接拷贝到目的地址，一般都会编码
	if(UNLIKELY(!base64_flag && !gb_flag)){
		memcpy(dst_tmp->eml_content, (dst_tmp->deal_flag).eml_content_head, 
				MIN(sizeof(dst_tmp->eml_content), (dst_tmp->deal_flag).eml_content_tail - 
				(dst_tmp->deal_flag).eml_content_head));
	}
	
	(dst_tmp->deal_flag).content_deal_flag = DATA_DEAL;

RESULT_PRINT("content", dst_tmp->eml_content);
	
	return YT_SUCCESSFUL;	
}

/*
*函数名称：plugin_real_deal_content_html
*函数功能：处理邮件主题内容
*输入参数：deal_data：邮件内容数据 plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_real_deal_content_html(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
	char *content_head = NULL;

	int base64_flag = 0;
	int gb_flag = 0;
	
	char *decode_temp=0;
	//先将后边的添加结束标志，防止接下来找编码无限找下去
	content_head = strstr(deal_data, PLUGIN_TAIL_TAIL);
	if(content_head){
		//*content_head = '\0';
		content_head = content_head + strlen(PLUGIN_TAIL_TAIL);
		
		(dst_tmp->deal_flag).eml_content_head = content_head;
		*((dst_tmp->deal_flag).eml_content_tail) = '\0';
		//接下来有附件要搜索附件了，把指针往下指   (need to modfiy in the next line)
		(dst_tmp->deal_flag).attachment_head = (dst_tmp->deal_flag).eml_content_tail + strlen(SMTP_TAIL);
	}
	//是base64先转码 UTF8的不用转
	if(strcasestr(deal_data, "Content-Transfer-Encoding: base64")){
		base64_flag = 1;
	}
	
	
	//是gb2312则转码
	if(LIKELY(strcasestr(deal_data, "gb2312")) || LIKELY(strcasestr(deal_data, "gb18030"))
			|| strcasestr(deal_data, "gbk")){
		gb_flag = 1;
	}
	
	//把尾巴改回来不然strncat拼接不上
	*((dst_tmp->deal_flag).eml_content_tail) = '\r';
	
	 /*(dst_tmp->deal_flag).eml_content_tail= strstr((dst_tmp->deal_flag).eml_content_tail+strlen(PLUGIN_TAIL_TAIL), PLUGIN_TAIL_TAIL);   
	*((dst_tmp->deal_flag).eml_content_tail+2) = '\0';*/
	 decode_temp=mempool_malloc(sizeof(dst_tmp->eml_content));
	if(decode_temp){
		//可能存在(dst_tmp->deal_flag).eml_content_head长度大于dst_tmp->eml_content,先不考虑
		if(base64_flag){
			if(content_type==CONTENT_TYPE_HTML){
				base64_decode((dst_tmp->deal_flag).eml_content_head,decode_temp);
				resolve_html_smtp(decode_temp,dst_tmp->eml_content,sizeof(dst_tmp->eml_content)-1);
		}
	
		if(gb_flag){
				convert_gb18030_to_utf8(decode_temp, sizeof(dst_tmp->eml_content));
				resolve_html_smtp(decode_temp,dst_tmp->eml_content,sizeof(dst_tmp->eml_content)-1);
			}
		}
	}
	else{
			return YT_FAILED;
	}

	mempool_free(decode_temp);


	*((dst_tmp->deal_flag).eml_content_tail) = '\r';

	//没编码的话就直接拷贝到目的地址，一般都会编码
	if(UNLIKELY(!base64_flag && !gb_flag)){
		memcpy(dst_tmp->eml_content, (dst_tmp->deal_flag).eml_content_head, 
				MIN(sizeof(dst_tmp->eml_content), (dst_tmp->deal_flag).eml_content_tail - 
				(dst_tmp->deal_flag).eml_content_head));
	}
	
	(dst_tmp->deal_flag).content_deal_flag = DATA_DEAL;

RESULT_PRINT("content", dst_tmp->eml_content);
	
	return YT_SUCCESSFUL;	
}

/*
*函数名称：plugin_deal_content
*函数功能：处理邮件内容
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码 
*输出参数：dst：存放处理数据
*返回值：  0 成功 -1错误
*/
static int plugin_deal_content(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe)
{   
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	if((dst_tmp->deal_flag).content_deal_flag == DATA_DEAL){
		return YT_SUCCESSFUL;
	}
	if((dst_tmp->deal_flag).content_deal_head == DATA_NO_DEAL){
		(dst_tmp->deal_flag).eml_content_head = strstr(deal_data, plugin_fe->offset_or_head);
		if((dst_tmp->deal_flag).eml_content_head){
			(dst_tmp->deal_flag).content_deal_head = DATA_DEAL;
		}
	}
	if((dst_tmp->deal_flag).content_deal_tail == DATA_NO_DEAL){
		if((dst_tmp->deal_flag).eml_content_head){

			(dst_tmp->deal_flag).eml_content_tail = 
				strstr((dst_tmp->deal_flag).eml_content_head, "\r\n\r\n--");

			if((dst_tmp->deal_flag).eml_content_tail){
				(dst_tmp->deal_flag).content_deal_tail = DATA_DEAL;				
			}
			else{
					(dst_tmp->deal_flag).eml_content_tail = 
					strstr((dst_tmp->deal_flag).eml_content_head, "\r\n\r\n");
		
				if((dst_tmp->deal_flag).eml_content_tail){
					(dst_tmp->deal_flag).content_deal_tail = DATA_DEAL;				
				}	
			}
		}		
	}
	//如果头尾都找到了那就进行内容的处理
	//如果头尾都找到了那就进行内容的处理
	if((dst_tmp->deal_flag).content_deal_head == DATA_DEAL && 
			(dst_tmp->deal_flag).content_deal_tail == DATA_DEAL){
		if(memcmp((dst_tmp->deal_flag).eml_content_head+strlen(plugin_fe->offset_or_head),
					"html",4)==0){
			content_type=CONTENT_TYPE_HTML;
			if(plugin_real_deal_content_html((dst_tmp->deal_flag).eml_content_head, dst, plugin_fe)){
				return YT_FAILED;
			}
		}else if(memcmp((dst_tmp->deal_flag).eml_content_head+strlen(plugin_fe->offset_or_head),
					"plain",5)==0){
			content_type=CONTENT_TYPE_PLAIN;
			if(plugin_real_deal_content_plain((dst_tmp->deal_flag).eml_content_head, dst, plugin_fe)){
				return YT_FAILED;
			}
		}else{
			return YT_FAILED;
		}
	}else{
		return YT_FAILED;
	}		

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_deal_attachment
*函数功能：处理邮件附件
*输入参数：deal_data：待处理数据  plugin_fe:截取特征码  
*输出参数：dst：存储结构体
*返回值：0 成功 -1 失败
*/
static int plugin_deal_attachment(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe) 
{
	struct mail_smtp *dst_tmp = (struct mail_smtp*)dst;
	
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
		if(plugin_data_extract(attachment_head + strlen(plugin_fe->offset_or_head), MAIL_ATTACHMENT_HEAD,MAIL_ATTACHMENT_TAIL, attachment_name_tmp, sizeof(attachment_name_tmp))){

			memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
		
			if(plugin_data_extract(attachment_head + strlen(plugin_fe->offset_or_head), MAIL_ATTACHMENT_HEAD_SUB,MAIL_ATTACHMENT_TAIL, attachment_name_tmp, sizeof(attachment_name_tmp))){
			memset(attachment_name_tmp, 0, sizeof(attachment_name_tmp));
			return YT_FAILED;
			
			}
		}

		{
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

RESULT_PRINT("attachment", dst_tmp->eml_attachment_name);

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_data_storage
*函数功能：拼接字符串，调用存储模块的写函数
*调用函数：无
*输入参数：smtp_dst：数据结构体 thread_num：线程号 plugin_id:插件号
*输出参数；无
*返回值：0 成功 -1 失败
*/
static int plugin_data_storage(const struct data_transit *dtts, struct mail_smtp *smtp_dst, const int thread_num, const int plugin_id)
{
	struct plugin_element element_tmp;
	memset(&element_tmp, 0, sizeof(struct plugin_element));
	
	const struct net_tuple *net_tuple_tmp = &((dtts->pkt_info).tuple);
	IPSTACK_GET_ALL_IP_MAC_PORT(&element_tmp, net_tuple_tmp);
	//	vlan_id
	char storage_buf[STORAGE_BUF_LEN + 1] = {0};

	char table_name[COMMON_LENGTH_128] = {0};
	sprintf(table_name, "%s%s", MAIL_PRIFIX, mail_suffix);

	if(FROM_SERVER_DATA == (dtts->pkt_info).client_or_server){
		char tmp[IP_LENGTH + 1] = {0};
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
			smtp_dst->smtp_feature->fprotocolid,
			smtp_dst->smtp_feature->fprotocolsubid,
			smtp_dst->smtp_feature->fprotocolname,
			smtp_dst->mail_server,
			smtp_dst->account,
			smtp_dst->password,
			smtp_dst->from,
			smtp_dst->to,
			smtp_dst->cc,
			smtp_dst->smtp_feature->action,
			smtp_dst->title,
			smtp_dst->eml_content,
			smtp_dst->eml_attachment_name,
			smtp_dst->eml_size,
			smtp_dst->eml_localpath,
			smtp_dst->date, 
			smtp_dst->time,
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
	//	log_write(LOG_ERROR, "smtp storage_unit_storage_data() error\n");
		return YT_FAILED;
	}
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_done
*函数功能：插件释放
*输入参数：smtp_dst：待释放数据结构体 
*输出参数；无
*返回值：  无
*/
static void plugin_done(struct mail_smtp *smtp_dst)
{	
	if(smtp_dst){
		if((smtp_dst->deal_flag).smtp_data){
			mempool_free((smtp_dst->deal_flag).smtp_data);
		}

		(smtp_dst->deal_flag).smtp_data = NULL;

		mempool_free(smtp_dst);
	}

}

/*
*函数名称：resolve_html_smtp
*函数功能: resolve html,get the content
*输入参数：html_str maxlen
*输出参数；dst_cont
*返回值：  success : YT_SUCCESSFUL
*/
static int resolve_html_smtp(char *html_str,char *dst_cont,int maxlen)
{
	char *p_head=0,*p_end=0;

	p_head=strstr(html_str,SMTP_HTML_CONT_HEAD);
	if(p_head){
		p_head+=strlen(SMTP_HTML_CONT_HEAD);
		p_end=strstr(p_head,SMTP_HTML_CONT_END);
		if(p_end>p_head){
			memcpy(dst_cont,p_head,MIN(p_end-p_head,maxlen));
		return YT_SUCCESSFUL;
		}
	}
	return YT_FAILED;
}
