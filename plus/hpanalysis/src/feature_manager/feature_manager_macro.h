/***********************************************************
*文件名：feature_manager_macro.h
*创建人：韩涛
*日  期：2013年11月17日
*描  述：特征码管理模块用到的宏
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _FEATURE_MANAGER_MACRO_H_
#define _FEATURE_MANAGER_MACRO_H_

#define LEN_BUF_LOG 1024 //sql语句长度
#define CODE_LENGTH 20 //主表code标识长度

//feature_data主表字段宏
#define FEATURE_DATA_GROUPID 1 
#define FEATURE_DATA_ACTION 3
#define FEATURE_DATA_CODE 4
#define FEATURE_DATA_FPROTOCOLNAME 7
#define FEATURE_DATA_FPROTOCOLID 6
#define FEATURE_DATA_FPROTOCOLSUBID 2

//mark_data匹配特征码表字段宏
#define MARK_DATA_FLAG 3
#define MARK_DATA_OFFSET_OR_HEAD 4
#define MARK_DATA_FEATURE 5 

//extract_data截取特征码表字段宏
#define EXTRACT_DATA_FLAG 2
#define EXTRACT_DATA_ENCODE 3
#define EXTRACT_DATA_OFFSET_OR_HEAD	4
#define EXTRACT_DATA_OFFSET_OR_TAIL 5
#define EXTRACT_DATA_FILED_NAME 6
#define EXTRACT_DATA_EXTRA 7

#endif
