/***********************************************************
*文件名：feature_manager_deal.c
*创建人：韩涛
*日  期：2013年10月17日
*描  述：特征码处理模块源文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#include "feature_manager_deal.h"
#include "feature_manager_macro.h"
#include "feature_manager_public_struct.h"

#include "sqlite_database.h"
#include <sqlite3.h>

#include "log.h"
#include "stdyeetec.h"

static int deal_row_num = 0;
struct plugin_feature *plugin_feature_deal_head = NULL;
static char **main_result = NULL;
static char **mark_result = NULL;
static char **extract_result = NULL;

/* 
*函数名称：feature_manager_deal_init 
*函数功能：处理初始化 
*输入参数：无 
*输出参数：无 
*返回值：  0 成功 -1 失败     
*/ 
int feature_manager_deal_init() 
{ 
	INFO_PRINT("Feature manager->Deal模块初始化\n");	
	INFO_PRINT("正在获取特征码.................\n");
	char *sql_feature = NULL; 
	sqlite3 *db = NULL;

	sqlite_database_open(&db);
	if(!db){
		log_write(LOG_ERROR, "feature_manager_deal sqlite_database_open error\n");
		return YT_FAILED;
	}
	
	sql_feature = "SELECT t_protocol_main.*, t_protocol_all.fprotocolid, t_protocol_all.fname FROM t_protocol_main, t_protocol_all WHERE t_protocol_main.fprotocolsubid = t_protocol_all.fprotocolsubid;";
     
	 //查询主表 
    if(sqlite_database_select(db, sql_feature, feature_manager_deal_main, NULL)){
		log_write(LOG_ERROR, "feature_manager_database_query() deal_main error\n");
		sqlite_database_close(db);
		return YT_FAILED;
	}  

	sqlite_database_close(db);

	INFO_PRINT("特征码获取完毕.................\n");

	return YT_SUCCESSFUL;    
}

/* 
*函数名称：feature_manager_deal_main 
*函数功能：处理主表特征码 
*输入参数：db: 数据库  row_data：一行数据 row_num：行数 callback_pram：回调函数所需参数
*输出参数：无
*返回值：  0 成功 -1 失败   
*/ 
int feature_manager_deal_main(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram)
{
	char *code = NULL; 
	char sql_mark[LEN_BUF_LOG] = {0}; 
	char sql_extract[LEN_BUF_LOG] = {0}; 
	int i = 0;
	int index = field_num;
	
	deal_row_num = row_num;
	main_result = row_data;

	
    if(!plugin_feature_deal_head){ 
		plugin_feature_deal_head = (struct plugin_feature*)malloc(sizeof(struct plugin_feature) * row_num);
	    if(!plugin_feature_deal_head){ 
			log_write(LOG_ERROR, "plugin_feature_head malloc() error\n"); 
		    return YT_FAILED; 
		}
	
		memset(plugin_feature_deal_head, 0, sizeof(struct plugin_feature));
    }
	


	for(i = 0; i < row_num; i ++){
		//初始化mark extract链表头
		INIT_LIST_HEAD(&(plugin_feature_deal_head[i].pmark_list_node));
		INIT_LIST_HEAD(&(plugin_feature_deal_head[i].pextract_list_node));
		
		//存储所需的字段
		//atoi过程在数据库建立时控制其字段不为NULL，防止段错误
		plugin_feature_deal_head[i].groupid = atoi(row_data[index + FEATURE_DATA_GROUPID]); 
		plugin_feature_deal_head[i].action = atoi(row_data[index + FEATURE_DATA_ACTION]);
		plugin_feature_deal_head[i].fprotocolname = row_data[index + FEATURE_DATA_FPROTOCOLNAME];
		plugin_feature_deal_head[i].fprotocolid = row_data[index + FEATURE_DATA_FPROTOCOLID];
		plugin_feature_deal_head[i].fprotocolsubid = row_data[index + FEATURE_DATA_FPROTOCOLSUBID];
	
		code =  row_data[index + FEATURE_DATA_CODE];

		index += field_num;

		//查询该行数据对应的mark表及extract表数据
		sprintf(sql_mark, "SELECT * FROM t_feature_mark WHERE fcode = '%s';", code); 
		sprintf(sql_extract, "SELECT * FROM t_feature_extract WHERE fcode = '%s';", code); 
	
		if(sqlite_database_select(db, sql_mark, feature_manager_deal_mark, 
			&(plugin_feature_deal_head[i].pmark_list_node))){
			log_write(LOG_ERROR, "feature_manager_database_query() deal_mark error\n");
			return YT_FAILED;
		}  
    
		if(sqlite_database_select(db, sql_extract, feature_manager_deal_extract, 
			&(plugin_feature_deal_head[i].pextract_list_node))){
			log_write(LOG_ERROR, "feature_manager_database_query() deal_extract error\n");
			return YT_FAILED;
		} 
	}

	return YT_SUCCESSFUL;
	
}

/* 
*函数名称：feature_manager_deal_mark 
*函数功能：处理匹配特征码表特征码 
*输入参数：db:没用仅仅为了回调一致性 row_data：一行数据 row_num：行数 callback_pram：回调函数所需参数
*输出参数：无
*返回值：  0 成功 -1 失败      
*/ 
int feature_manager_deal_mark(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram)
{
	
	int i = 0;
	int index = field_num;
	mark_result = row_data;

	struct list_head *pmark_list_node = (struct list_head*)callback_pram;
	
	for(i = 0; i < row_num; i++){
		//分配mark空间
		struct plugin_feature_mark *plugin_feature_mark_data = (struct plugin_feature_mark*)malloc(sizeof(struct plugin_feature_mark));
		if(!plugin_feature_mark_data){
			log_write(LOG_ERROR, "plugin_feature_mark_data malloc() error\n");
			return YT_FAILED;
		}

		memset(plugin_feature_mark_data, 0, sizeof(struct plugin_feature_mark));

		//存储所需字段
		plugin_feature_mark_data->flag = atoi(row_data[index + MARK_DATA_FLAG]); 
		plugin_feature_mark_data->offset_or_head = row_data[index + MARK_DATA_OFFSET_OR_HEAD];
		plugin_feature_mark_data->feature = row_data[index + MARK_DATA_FEATURE];
	
		//在这里加判断如果为空值，置NULL，插件将跳过该字段匹配，提高效率
		if(!strlen(row_data[index + MARK_DATA_OFFSET_OR_HEAD])){
			plugin_feature_mark_data->offset_or_head = NULL;
		}
		
		if(!strlen(row_data[index + MARK_DATA_FEATURE])){
			plugin_feature_mark_data->feature = NULL;
		}

		//结果加到链表
		list_add_tail(&(plugin_feature_mark_data->pmark_list_node), pmark_list_node);

		index += field_num;
	}

	return YT_SUCCESSFUL;
}

/* 
*函数名称：feature_manager_deal_extract 
*函数功能：处理截取特征码表特征码 
*输入参数：db:没用仅仅为了回调一致性 row_data：一行数据 row_num：行数 callback_pram：回调函数所需参数
*输出参数：无
*返回值：  0 成功 -1 失败      
*/ 
int feature_manager_deal_extract(sqlite3 *db, char **row_data, int row_num, int field_num, void *callback_pram)
{
	int i = 0;
	int index = field_num;
	struct list_head *pextract_list_node = (struct list_head*)callback_pram;
	extract_result = row_data;
	
	for(i = 0; i < row_num; i++){
		//分配mark空间
		struct plugin_feature_extract *plugin_feature_extract_data = (struct plugin_feature_extract*)malloc(sizeof(struct plugin_feature_extract));
		if(!plugin_feature_extract_data){
			log_write(LOG_ERROR, "plugin_feature_extract_data malloc() error\n");
			return YT_FAILED;
		}
	
		memset(plugin_feature_extract_data, 0, sizeof(struct plugin_feature_extract));
		
		//存储所需字段
		plugin_feature_extract_data->flag = atoi(row_data[index + EXTRACT_DATA_FLAG]); 
		plugin_feature_extract_data->encode = row_data[index + EXTRACT_DATA_ENCODE]; 
		plugin_feature_extract_data->field_name = row_data[index + EXTRACT_DATA_FILED_NAME];
		plugin_feature_extract_data->offset_or_head = row_data[index + EXTRACT_DATA_OFFSET_OR_HEAD];
		plugin_feature_extract_data->offset_or_tail = row_data[index + EXTRACT_DATA_OFFSET_OR_TAIL];
	
		//在这里加判断如果为空值，置NULL，插件将跳过该字段匹配，提高效率
		if(!strlen(row_data[index + EXTRACT_DATA_OFFSET_OR_HEAD])){
			plugin_feature_extract_data->offset_or_head = NULL;
		}
	
		if(!strlen(row_data[index + EXTRACT_DATA_OFFSET_OR_TAIL])){
			plugin_feature_extract_data->offset_or_tail = NULL;
		}

		//结果加到链表
		list_add_tail(&(plugin_feature_extract_data->pextract_list_node), pextract_list_node);

		index += field_num;
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_manager_deal_get
*函数功能：获取特征码全局头和行数
*输入参数：无
*输出参数：row_num
*返回值：plugin_feature指针
*/
struct plugin_feature *feature_manager_deal_get(int *row_num)
{
	//输出行数
	*row_num = deal_row_num;
	
	//返回头指针
	return plugin_feature_deal_head;
}

/*
*函数名称：feature_manager_deal_destroy
*函数功能：deal模块释放函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void feature_manager_deal_destroy()
{
	INFO_PRINT("Feature manager->Deal模块释放\n");	
	int i = 0;
	struct list_head *temp_node  = NULL;
	while(i < deal_row_num){
		while(!list_empty(&(plugin_feature_deal_head[i].pmark_list_node))){
			temp_node = (plugin_feature_deal_head[i].pmark_list_node).next;
			list_del(temp_node);
			free(temp_node);
		}
	
		while(!list_empty(&(plugin_feature_deal_head[i].pextract_list_node))){
			temp_node = (plugin_feature_deal_head[i].pextract_list_node).next;
			list_del(temp_node);
			free(temp_node);
		}
		
		i++;
	}
	
	free(plugin_feature_deal_head);
	plugin_feature_deal_head = NULL;

	sqlite_database_free(main_result);
	sqlite_database_free(mark_result);
	sqlite_database_free(extract_result);
}
