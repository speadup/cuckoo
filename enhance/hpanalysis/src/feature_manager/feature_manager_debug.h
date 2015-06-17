/***********************************************************
*文件名：feature_manager_debug.h
*创建人：韩涛
*日  期：2014年1月13日
*描  述：特征码管理模块debug
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/
#ifndef _FEATURE_MANAGER_DEBUG_H_
#define _FEATURE_MANAGER_DEBUG_H_

////////////////////////////数据库连接信息 DEBUG////////////////////////////////////
#ifdef DB_DEBUG_PRINT

#define DB_INFO_PRINT(yt_config) fprintf(stdout, "feature_manager %s [%d line]数据库连接信息:\n\
host:%s\nuser_name:%s\npassword:%s\nname:%s\nport:%d\n", __FILE__, __LINE__, yt_config->fm_db_host,\
yt_config->fm_db_username, yt_config->fm_db_password, yt_config->fm_db_name,\
yt_config->fm_db_port)

#else

#define DB_INFO_PRINT(yt_config)

#endif



#endif
