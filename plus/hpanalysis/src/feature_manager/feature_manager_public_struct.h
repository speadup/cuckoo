
/***********************************************************
*文件名：feature_manager_public_struct.h
*创建人：韩涛
*日  期：2013年10月17日
*描  述：对外及对内提供特征码结构体
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef FEATURE_MANAGER_PUBLIC_STRUCT_H_
#define FEATURE_MANAGER_PUBLIC_STRUCT_H_

#define BUF_FEATURE_LENGTH 64
#include "list.h"
#include <stdlib.h>

struct plugin_feature_extract;

typedef int (extract_callback)(char *deal_data, void *dst, struct plugin_feature_extract *plugin_fe);

//特征码相关结构体
struct plugin_feature{
	struct list_head list_node;
	char *fprotocolid;										//协议id 
	char *fprotocolsubid;							        //协议的subid 
	char *fprotocolname;									//协议名称
	int groupid;											//插件对应的groupid
	int action;												//特征码所对应的用户操作
	struct list_head pmark_list_node;						//匹配特征码列表
	struct list_head pextract_list_node;					//截取特征码列表
};

struct plugin_feature_mark{
	struct list_head pmark_list_node;						//匹配特征码列表
	int flag;                                               //标识匹配方式：[0-偏移] [1-结构化] [2-全文]
	char *offset_or_head;									//偏移量或者结构需要的头标志
	char *feature;											//用来匹配的特征码
};

struct plugin_feature_extract{
	struct list_head pextract_list_node;;					//截取特征码列表
	int flag;												//标识匹配方式：[0-偏移] [1-结构化] [2-全文]
	char *field_name;										//字段名
//	int server_or_client;
	extract_callback *excb;									//截取回调函数
	char *encode;												//编码方式
	char *offset_or_head;									//偏移量或者结构需要的头标志
	char *offset_or_tail;									//偏移量或者结构需要的尾标志
};

#endif

