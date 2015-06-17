
/***********************************************************
*文件名：storage_unit_database.c
*创建人：韩涛
*日  期：2013年11月5日
*描  述：存储模块数据库操作源文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#include "stdyeetec.h"
#include "sqlite_database.h"
#include "plugin_manager.h"
#include <sqlite3.h>
#include "list.h"
#include "mempool.h"

//一个线程链表头结构
struct list_struct{
	int thread_num;
	struct list_head *list_head;
	struct list_head *list_head_storage;
	struct list_head *list_head_tmp;
};

//插入语句节点
struct sql_struct{
	struct list_head list_head;
	char *sql;
};

//全局链表头
struct list_struct *list_struct_head = NULL;

//数据库
static sqlite3 *db = NULL;

//超时标志
static int overtime_flag = 0;

/*
*函数名称：storage_unit_database_init
*函数功能：存储模块数据库操作初始化
*输入参数：无
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int storage_unit_database_init()
{
	INFO_PRINT("Storage_unit_database 模块初始化\n");
	sqlite3 *db_tmp = NULL;
	int nresult = 0;
	int i = 0;

	list_struct_head = (struct list_struct*)malloc(sizeof(struct list_struct) * plugin_manager_thread_num);
	if(!list_struct_head){
		log_write(LOG_ERROR, "storage_unit_database_init malloc error\n");
		return YT_FAILED;
	}

	for(i = 0; i < plugin_manager_thread_num; i++){
		list_struct_head[i].thread_num = i;
		
		(list_struct_head[i]).list_head = (struct list_head*)malloc(sizeof(struct list_head));
		if(!(list_struct_head[i]).list_head){
			log_write(LOG_ERROR, "list_head malloc error\n");
			return YT_FAILED;
		}
		
		(list_struct_head[i]).list_head_tmp = (struct list_head*)malloc(sizeof(struct list_head));
		if(!(list_struct_head[i]).list_head_tmp){
			log_write(LOG_ERROR, "list_head_tmp malloc error\n");
			return YT_FAILED;
		}
		
		INIT_LIST_HEAD((list_struct_head[i]).list_head);
		INIT_LIST_HEAD((list_struct_head[i]).list_head_tmp);

		list_struct_head[i].list_head_storage = NULL;
	}

	nresult = sqlite_database_open(&db_tmp);
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "storage_unit_database_init sqlite_database_open error\n");
		DEBUG_PRINT("sqlite_database_open error\n");
		return YT_FAILED;
	}

	db = db_tmp;
	nresult = sqlite_database_begin_transaction(db);
	if(nresult != SQLITE_OK){
		log_write(LOG_ERROR, "storage_unit_begin_transaction error\n");
		return YT_FAILED;
	}
	
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：storage_unit_database_insert
*函数功能：存储模块数据库记录插入函数
*输入参数：sql:待插入记录 thread_num:线程号
*输出参数：无
*返回值：  0 成功 -1 失败
*/
int storage_unit_database_insert(char *sql, const int thread_num)
{
	if(!sql){
		return YT_FAILED;
	}

	struct sql_struct *sql_tmp = (struct sql_struct*)mempool_malloc(sizeof(struct sql_struct));
	if(!sql_tmp){
		log_write(LOG_ERROR, "storage_unit_database_insert malloc error\n");
		return YT_FAILED;
	}

	sql_tmp->sql = (char*)mempool_malloc(strlen(sql) + 1);
	if(!(sql_tmp->sql)){
		log_write(LOG_ERROR, "sql_tmp->sql malloc error\n");
		return YT_FAILED;
	}

	DEBUG_PRINT ("%s\n",sql);
	memcpy(sql_tmp->sql, sql, strlen(sql));

	list_add_tail(&(sql_tmp->list_head), (list_struct_head[thread_num]).list_head);
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：storage_unit_database_overtime_deal
*函数功能：存储模块数据库操作超时处理函数
*输入参数：无
*输出参数：无
*返回值：  0成功 -1失败
*/
int storage_unit_database_overtime_deal()
{
	int i = 0;
	struct list_head *list_node = NULL;
	struct sql_struct *sql_tmp = NULL;

	//打一个时间差，替换之后，待下一次超时才进行处理
	//主要防止数据未挂载完，还能防止同一时刻放数据和取数据
	if(!overtime_flag){

		for (i=0;i<plugin_manager_thread_num;i++) {
			(list_struct_head[i]).list_head_storage = (list_struct_head[i]).list_head;
			(list_struct_head[i]).list_head = (list_struct_head[i]).list_head_tmp;
			(list_struct_head[i]).list_head_tmp = (list_struct_head[i]).list_head_storage;
		}
		
		overtime_flag = 1;
		
		return YT_SUCCESSFUL;
	}

	//将数据插入事物中
	for(i = 0; i < plugin_manager_thread_num; i++){
		if(list_struct_head[i].list_head_storage){
			while(!list_empty((list_struct_head[i]).list_head_storage)){
				list_node = (((list_struct_head[i]).list_head_storage))->next;
				sql_tmp = (struct sql_struct*)list_node;
				
				sqlite_database_insert(db, sql_tmp->sql);
				
				//DEBUG_PRINT ("%s\n\n",sql_tmp->sql);
				mempool_free(sql_tmp->sql);
				
				list_del(list_node);
				mempool_free(sql_tmp);
			}
		}
	}
	
	//提交并且开启事物
	sqlite_database_commit_transaction(db);
	sqlite_database_begin_transaction(db);

	//置位
	overtime_flag = 0;

	return YT_SUCCESSFUL;
}

/*
*函数名称：storage_unit_database_destory
*函数功能：存储模块数据库操作释放函数
*输入参数：无
*输出参数：无
*返回值：  无
*/
void storage_unit_database_destory()
{
	INFO_PRINT("storage_unit_database 模块释放\n");
	
	int i = 0;
	
	sqlite_database_close(db);
	storage_unit_database_overtime_deal();

	for(i = 0; i < plugin_manager_thread_num; i++){
		free((list_struct_head[i]).list_head);
		free((list_struct_head[i]).list_head_tmp);
	}

	free(list_struct_head);
}
