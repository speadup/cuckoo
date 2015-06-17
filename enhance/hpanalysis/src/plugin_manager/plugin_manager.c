#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <dirent.h>
#include <dlfcn.h>
#include <arpa/inet.h>

#include "plugin_manager.h"
#include "plugin_manager_debug.h"

#include "log.h"
#include "list.h"
#include "config.h"
#include "stdyeetec.h"
#include "data_transit.h"
#include "tcp_session.h"
#include "pthread_affinity.h"

#define MAX_PORT 65535

// thread info 

// reserve dlopen result
struct dl_node {
	void *handle;
	struct dl_node *next;
};

#define PER_CORE_THREADS 1
#define MAX_THREAD 16

struct thread_info{
	pthread_t thread_id;
	int thread_num;
	int lcore;
};

static struct thread_info pmti[YT_MAX_LCORE * PER_CORE_THREADS];	// thread info list

int plugin_manager_thread_num = 0;

static struct plugin_info *plugin_list[MAX_PORT];	// plugin info list

static struct dl_node *dl_list; // all dlopen result


/*
 *函数名称: plugin_manager_register
 *函数功能: register plugin
 *输入参数: ppi		plugin info point
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_register(struct plugin_info * ppi)
{
	struct plugin_info *pi;
	struct plugin_info *list;
	
	// store plugin info
	if (!ppi) {
		log_write(LOG_ERROR, "plugin_info is NULL\n");
		return YT_FAILED;	
	}

	pi = malloc(sizeof(*ppi));
	if (!pi) {
		log_write(LOG_ERROR, "malloc return NULL\n");
		return YT_FAILED;	
	}

	memcpy(pi, ppi, sizeof(*ppi));

	if (pi->port > MAX_PORT) {
		log_write(LOG_ERROR, "plugin port is large than MAX_PORT\n");
		return YT_FAILED;	
	}

	// insert into plugin list
	if (plugin_list[pi->port] == NULL) {
		plugin_list[pi->port] = pi;	
	} else {
		list = plugin_list[pi->port];
		while(list->next) {
			list = list->next;
		}

		list->next = pi;
	}

	return YT_SUCCESSFUL;
}

/*
 *函数名称: plugin_manager_load
 *函数功能: load plugin
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
static int plugin_manager_load(void)
{
	int is_url_plugin = 0;
	void *handle;
	DIR *dirp;
	struct dirent *direntp;
	char url_plugin_name[MAX_FILE_NAME_LENGTH];
	char plugin_file_path[MAX_FILE_PATH_LENGTH + MAX_FILE_NAME_LENGTH + 1];
	char url_plugin_file_path[MAX_FILE_PATH_LENGTH + MAX_FILE_NAME_LENGTH + 1];
	struct dl_node *dl;

	// Get config info
	struct yt_config *config_info;
	config_info = base_get_configuration();

	dirp = opendir(config_info->plugin_dir_path);
	if (!dirp) {
		log_write(LOG_ERROR, "plugin_load: can't open the plugin_dir\n");
		return YT_FAILED;
	}

	/* 为了解决url信息不会重复入库和每个会话只能属于一个插件的问题
	 * 这里将url插件放到最后一个进行处理,使得先判断search和httppost
	 * 若两者都没能处理成功，最后再进入url插件进行处理*/
	while ((direntp = readdir(dirp))) {
		if (!strcmp(direntp->d_name, ".")
			|| !strcmp(direntp->d_name, "..")
			|| !strstr(direntp->d_name, ".so"))			
			continue ;

		// get plugin full path
		memset(plugin_file_path, 0, sizeof(plugin_file_path));
		strcpy(plugin_file_path, config_info->plugin_dir_path);
		strcat(plugin_file_path, "/");
		strcat(plugin_file_path, direntp->d_name);

		if (!strcmp(direntp->d_name, "plugin_url.so")) {
			is_url_plugin = 1;	
			strcpy(url_plugin_name, direntp->d_name);
			strcpy(url_plugin_file_path, plugin_file_path);
			continue;
		}

		// load plugin, plugin will init himself
		handle = dlopen(plugin_file_path, RTLD_NOW);
		if (!handle) {
			log_write(LOG_ERROR, "plugin_load: load plugin [%s] error\n",
					plugin_file_path);	
			continue ;
		}

		dl = (struct dl_node*)malloc(sizeof(struct dl_node));
		if(dl == NULL) {
			log_write(LOG_INFO, "alloc dl_node is failed\n");
			return YT_FAILED;
		}

		memset(dl, 0, sizeof(struct dl_node));
		dl->handle = handle;
		dl->next = dl_list;
		dl_list = dl;

		INFO_PRINT("Load plugin [%s] success\n", direntp->d_name);
	}

	if (is_url_plugin) {
		handle = dlopen(url_plugin_file_path, RTLD_NOW);
		if (!handle) {
			log_write(LOG_ERROR, "plugin_load: load plugin [%s] error\n",
					plugin_file_path);	
		} else {
			dl = (struct dl_node*)malloc(sizeof(struct dl_node));
			if(dl == NULL) {
				log_write(LOG_INFO, "alloc dl_node is failed\n");
				return YT_FAILED;
			}

			memset(dl, 0, sizeof(struct dl_node));
			dl->handle = handle;
			dl->next = dl_list;
			dl_list = dl;
			INFO_PRINT("Load plugin [%s] success\n", url_plugin_name);
		}
	}

	closedir(dirp);
	return YT_SUCCESSFUL;
}

/*
 *函数名称: plugin_manager_switch
 *函数功能: choice packet direction,get plugin info struct to deal
 *输入参数: dt	packet struct point
 *输出参数: none
 *返回值  : on success, return plugin info struct. on error, NULL is returned.
*/
static struct plugin_info* plugin_manager_switch(struct data_transit* dt)
{
	unsigned short source = ntohs(((dt->pkt_info).tuple).port_source);
	unsigned short dest = ntohs(((dt->pkt_info).tuple).port_dest);

	if (UNLIKELY(!dt || source  >= MAX_PORT || dest >= MAX_PORT)) {
		return NULL;	
	}

	if (FROM_CLIENT_DATA == dt->pkt_info.client_or_server) {
		return plugin_list[dest];
	}

	if (FROM_SERVER_DATA == dt->pkt_info.client_or_server) {
		return plugin_list[source];
	}

	return NULL;
}

/*
 *函数名称: plugin_manager_loop
 *函数功能: call plugin deal packet
 *输入参数: arg		arguments
 *输出参数: none
 *返回值  : NULL
*/
static void *plugin_manager_loop(void *arg)
{
	int ret;
	struct data_transit *dt = NULL;
	struct plugin_info *list = NULL;
	struct thread_info *info_tmp = (struct thread_info*)arg;
	struct session_deal_record *deal_record = NULL;
	
	struct list_head deal_head;

	INIT_LIST_HEAD(&deal_head);

	// Set cpu affinity
	pthread_affinity_set(info_tmp->lcore);

	while (global_shared.continue_run) {
		// get data packet
		ret = data_transit_get(&deal_head, &deal_record);
		if(ret){
			usleep(100);
			continue;
		}

		//遍历链表处理缓存包
		list_for_each_entry(dt, &deal_head, deal_node){		

//			if (dt->deal_record && dt->deal_record->func_plugin_deal) {
//				dt->deal_record->func_plugin_deal (dt, info_tmp->thread_num);
//				continue;
//			}

			// protocol switch
			list = plugin_manager_switch(dt);

			// call plugin deal func loop deal
			while(list) {
				ret = list->func_plugin_deal(dt, info_tmp->thread_num);
				if (ret == YT_SUCCESSFUL) {
					if (dt->deal_record) {
						dt->deal_record->func_plugin_deal = list->func_plugin_deal;
					}

					break;
				}
				list = list->next;
			}
		}

		// free data packet
		data_transit_free(&deal_head, deal_record);
	}
	
	return NULL;
}

/*
 *函数名称: plugin_manager_smp
 *函数功能: SMP deal
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
static int plugin_manager_smp(void)
{
	int ret = 0;
  	unsigned j,n;
    unsigned long long cm;

	struct yt_config *config_info;

	// get config info
	config_info = base_get_configuration();
	if (!config_info) {
		return YT_FAILED;
	}

	// parse hexadecimal string 
	cm = config_info->datadeal_coremask;

	// set active core role and start thread */
	for (j = 0; j < YT_MAX_LCORE; j++) {
	    if ((1ULL << j) & cm) {
			for(n = 0; n < PER_CORE_THREADS; n++){
				INFO_PRINT("create pthread num: %d\n",plugin_manager_thread_num);

				pmti[plugin_manager_thread_num].thread_num = plugin_manager_thread_num;
				pmti[plugin_manager_thread_num].lcore = j;
				
				ret = pthread_create(&(pmti[plugin_manager_thread_num].thread_id), NULL,
						plugin_manager_loop, &(pmti[plugin_manager_thread_num]));
				
				plugin_manager_thread_num++; // thread num
				
				if (ret != 0) {
					log_write(LOG_ERROR,
						"pthread_create error - %s\n",
						strerror(ret));
					continue;
				}
			}
	    }
	}

	return YT_SUCCESSFUL;
}

/*
 *函数名称: plugin_manager_init
 *函数功能: load plugins and start smp deal
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_init(void)
{
	INFO_PRINT("\n----->plugin_manager模块初始化..........\n");
	dl_list = NULL;

	// load plugins
	if (plugin_manager_load() == YT_FAILED) {
		log_write(LOG_ERROR, "plugin_manager_load error\n");	
		return YT_FAILED;
	}

	// start smp deal
	if (plugin_manager_smp() == YT_FAILED) {
		log_write(LOG_ERROR, "plugin_manager_smp error\n");	
		return YT_FAILED;
	}

	//计数器
	global_shared.retain_count ++;

	return YT_SUCCESSFUL;
}

/*
 *函数名称: plugin_manager_destroy
 *函数功能: stop plugin manager deal
 *输入参数: none
 *输出参数: none
 *返回值  : on success, return 0(YT_SUCCESSFUL). on error, -1(YT_FAILED) is returned.
*/
int plugin_manager_destroy(void)
{
	void *res;
	int i,ret;
	struct plugin_info *pi;
	struct plugin_info *tp;
	struct dl_node *dl = dl_list;

	INFO_PRINT("\n----->plugin_manager模块释放..........\n");
	
	// wait stop
	for (i = 0; i < plugin_manager_thread_num; i++) {
		if (pmti[i].thread_id) {
			ret = pthread_join(pmti[i].thread_id, &res);
			if (ret != YT_SUCCESSFUL) {
				log_write(LOG_ERROR,
				"end_plugin_frame_deal_thread:\
				pthread_join error - %s\n", strerror(ret));
				continue ;
			}
		}
	}

	// free plugin info
	for (i = 0; i < MAX_PORT; i++) {
		if (plugin_list[i] != NULL) {
			for (pi = plugin_list[i]; pi != NULL; pi = tp) {
				tp = pi->next;
				free(pi);
			}
			plugin_list[i] = NULL;
		}
	}

	while(dl) {
		dl_list = dl_list->next;
		dlclose(dl->handle);
		free(dl);
		dl = dl_list;
	}

	//计数器
	global_shared.retain_count --;

	return YT_SUCCESSFUL;
}
