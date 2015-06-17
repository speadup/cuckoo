/***********************************************************
*文件名：feature_manager_place.c
*创建人：韩涛
*日  期：2013年12月2日
*描  述：place信息获取源文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#include "feature_manager_place.h"
#include "sqlite_database.h"
#include "stdyeetec.h"

char place_info[2][256];
static char **place_result = NULL;

/*
*函数名称：feature_mamager_place_init
*函数功能：place信息获取初始化
*输入参数：无
*输出参数：无
*返回值：0 成功 -1 失败
*/
int feature_manager_place_init()
{
	INFO_PRINT("Feature manager->Place模块初始化\n");
	memset(place_info[0], 0, 256);
	memset(place_info[1], 0, 256);

	sqlite3 *db = NULL;

	sqlite_database_open(&db);
	if(!db){
		log_write(LOG_ERROR, "feature_manager_place_init sqlite_database_open error\n");
		return YT_FAILED;
	}

	char *sql = "select fplacecode,fplacename from t_place";
	
	if(sqlite_database_select(db, sql, feature_manager_place_deal, NULL)){
		log_write(LOG_ERROR, "feature_manager_database_query() error\n");
		return YT_FAILED;
	}

	sqlite_database_close(db);

	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_mamager_place_deal
*函数功能：place信息处理函数
*输入参数：db:没用 仅为了回调一致性 row_data：数据库查询结果 row_num：行数 callback_pram：回调参数
*输出参数：无
*返回值：0 成功 -1 失败
*/
int feature_manager_place_deal(sqlite3 *db, char **row_data, int row_num, int ncolumn, void *callback_pram)
{
	if(!row_data){
		log_write(LOG_ERROR, "feature_manager_place_deal() error\n");
		return YT_FAILED;
	}

	place_result = row_data;

	memcpy(place_info[0], row_data[2], strlen(row_data[2]));
	memcpy(place_info[1], row_data[3], strlen(row_data[3]));

	return YT_SUCCESSFUL;
}

/*
*函数名称：feature_mamager_place_destory
*函数功能：place信息释放
*输入参数：无
*输出参数：无
*返回值：  无 
*/
void feature_manager_place_destory()
{
	INFO_PRINT("Feature manager->Place模块释放\n");
	sqlite_database_free(place_result);
}
