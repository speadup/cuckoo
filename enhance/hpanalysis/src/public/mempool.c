/***********************************************************
 *文件名：mempool.c
 *创建人：小菜_默
 *日  期：2013年10月25日
 *描  述：mallco mempool process
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 ************************************************************/

#include "mempool.h"
#include "stdyeetec.h"

#include "list.h"
#include "public_debug.h"
#include "atomic.h"
#include "config.h"

#include <pthread.h>
#include <string.h>
#include <malloc.h>

#define MEMPOOL_MALLOC_SIZE_32   32
#define MEMPOOL_MALLOC_SIZE_64   64
#define MEMPOOL_MALLOC_SIZE_128  128
#define MEMPOOL_MALLOC_SIZE_256  256
#define MEMPOOL_MALLOC_SIZE_512  512
#define MEMPOOL_MALLOC_SIZE_1024 1024
#define MEMPOOL_MALLOC_SIZE_2048 2048
#define MEMPOOL_MALLOC_SIZE_4096 4096
#define MEMPOOL_MALLOC_SIZE_8192 8192
#define MEMPOOL_MALLOC_SIZE_256K 262144 //(256*1024)

#define MEMPOOL_COUNT 9      //32到8192共9级

#define MEMPOOL_IS_MALLOC 0
#define MEMPOOL_IS_FREE   1
#define MEMPOOL_MEM_CHECK "yeetec"
#define MEM_CHECK_SIZE sizeof(MEMPOOL_MEM_CHECK)

//没块内存的头结构体
#pragma pack(push)
#pragma pack(1)
struct mempool_data
{
	struct list_head list_node;    //内存的挂载点
	int   malloc_size;             //内存分配的大小
	unsigned long mem_number;      //该内存块编号
	char  mf_flag;                 //标志是已经malloc或者已经free
	char  malloc_data[0];          //使用的内存区域
};
#pragma pack(pop)

//内存池的头
struct mempool_index
{
	struct list_head mempool_data_head;      //内存池链表头
	int malloc_size;                         //该链表的每块内存区大小
	atomic_t total_count;                    //内存分配总数
	atomic_t residue_count;                  //剩余空闲内存数量
};

//malloc or free (mf) call back func
typedef void *(mf_call_back)(struct mempool_index *mempool_index,
		                     struct mempool_data *mp_data);

static inline void *switch_memdata_size(const int malloc_size, struct mempool_data *mp_data,
		                                mf_call_back *malloc_or_free);

static void *mempool_data_malloc(struct mempool_index *mempool_index,
		                         struct mempool_data *mp_data);

static void *mempool_data_free(struct mempool_index *mempool_index,
		                       struct mempool_data *mp_data);

static void *mempool_other_malloc(int malloc_size);

static void mempool_other_free(void *free_mem);

//全局的内存池索引
static struct mempool_index gmempool_index_head[MEMPOOL_COUNT+1];
//自旋锁
pthread_spinlock_t gspin_lock,*gmempool_spin_lock = &gspin_lock;

//内存统计
atomic_t gmempool_total_memory;                  //统计总内存大小
atomic_t gmem_data_number;                       //分配总的内存块数

/*
 *函数名称：mempool_init
 *函数功能：mempool init
 *输入参数：none
 *输出参数：none
 *返回值：  on success, return 0(YT_SUCCESS). on error, -1(YT_FAILED) is returned.
*/
int mempool_init()
{
	INFO_PRINT("Public->Mempool模块初始化\n");
	
	pthread_spin_init(gmempool_spin_lock,0);
	atomic_set(&gmempool_total_memory,0);
	atomic_set(&gmem_data_number,0);

	memset (gmempool_index_head,0,sizeof(struct mempool_index));

	int malloc_size = MEMPOOL_MALLOC_SIZE_32;

	int i = 0;
	for(i=0;i<MEMPOOL_COUNT;i++){
		gmempool_index_head[i].malloc_size = malloc_size;

		atomic_set (&(gmempool_index_head[i].total_count),0);
		atomic_set (&(gmempool_index_head[i].residue_count),0);

		INIT_LIST_HEAD(&(gmempool_index_head[i].mempool_data_head));  //初始化缓冲区链表头
		
		malloc_size += malloc_size;
	}

	gmempool_index_head[i].malloc_size = MEMPOOL_MALLOC_SIZE_256K;
	INIT_LIST_HEAD(&(gmempool_index_head[i].mempool_data_head));

	return YT_SUCCESSFUL;
}

/*
 *函数名称：mempool_destroy
 *函数功能：destroy mempool module
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void mempool_destroy()
{
	INFO_PRINT("Public->Mempool模块释放\n");

	struct list_head *data_node  = NULL;
	int i=0;
	for(i=0;i<MEMPOOL_COUNT+1;i++){
		while(!list_empty(&(gmempool_index_head[i].mempool_data_head))){ 

			//删除索引链表中的数据链表
			data_node = gmempool_index_head[i].mempool_data_head.next;
			list_del(data_node);

			free(data_node);
			data_node = NULL;
		}
	}

	pthread_spin_destroy(gmempool_spin_lock);
}

/*
 *函数名称：mempool_malloc
 *函数功能：mempool return malloc_size memory
 *输入参数：malloc_size        malloc size
 *输出参数：none
 *返回值：  return malloc size space.
*/
void *mempool_malloc(const int malloc_size)
{
	if(UNLIKELY(malloc_size<=0)){
		return NULL;
	}
	
	void *temp_data = switch_memdata_size(malloc_size,0,mempool_data_malloc);
	
	return temp_data;
}

/*
 *函数名称：mempool_realloc
 *函数功能：realloc size memory
 *输入参数：mem_data           malloc memory allocation
 *          realloc_size       realloc size
 *输出参数：none
 *返回值：  return realloc size space
*/
void *mempool_realloc(void *mem_address,const int realloc_size)
{
	if(UNLIKELY(!mem_address)){
		return mempool_malloc(realloc_size);
	}

	struct mempool_data *mp_data = (struct mempool_data*)(mem_address - sizeof(struct mempool_data)); 
	if(mp_data->malloc_size >= realloc_size){
		return mem_address;
	}

	void *realloc_data = mempool_malloc(realloc_size);
	if (UNLIKELY(!realloc_data)){
		log_write(LOG_ERROR,"[%s][%s][%d] 内存分配失败 malloc size :%d\n",__FILE__,__func__,__LINE__,realloc_size);
		mempool_free(mem_address);
		return NULL;
	}

	memcpy(realloc_data,mem_address,mp_data->malloc_size);
	mempool_free(mem_address);
	
	return realloc_data;
}

/*
 *函数名称：mempool_free
 *函数功能：free malloc or realloc allocation memory
 *输入参数：mem_free           free memory pointer
 *输出参数：none
 *返回值：  none
*/
void mempool_free(void *free_mem)
{
	if(UNLIKELY(!free_mem)){
		return ;
	}

	struct mempool_data *mp_data = (struct mempool_data*)(free_mem - sizeof(struct mempool_data)); 

	switch_memdata_size(mp_data->malloc_size,
			            mp_data,mempool_data_free);
}

/*
 *函数名称：mempool_data_malloc
 *函数功能：从链表中分配数据
 *输入参数：mempool_index      要从该节点分配内存
 *          mempool_data       保留
 *输出参数：none
 *返回值：  void *
*/
static void *mempool_data_malloc(struct mempool_index *mempool_index,struct mempool_data *mp_data)
{
	struct list_head *temp_node    = NULL;
	struct mempool_data *temp_data = NULL;

	pthread_spin_lock(gmempool_spin_lock);
	//链表为空，跳到下面分配新内存
	if(!list_empty(&(mempool_index->mempool_data_head))){
		//不为空从链表中获取内存
		temp_node = mempool_index->mempool_data_head.next;
		list_del(temp_node);

		pthread_spin_unlock(gmempool_spin_lock);

		temp_data = (struct mempool_data*)temp_node;
		temp_data->mf_flag = MEMPOOL_IS_MALLOC;

		MEMPOOL_PRINT("Malloc mem data number :%lu, mem size:%d, pthread id:%lu\n",
			   temp_data->mem_number,temp_data->malloc_size, pthread_self());

		//剩余个数-1
		atomic_dec(&(mempool_index->residue_count));
		memset (temp_data->malloc_data,0,temp_data->malloc_size);
		return temp_data->malloc_data;
	}
	pthread_spin_unlock(gmempool_spin_lock);

	//判断程序使用内存的大小，若大于指定内存大小则不在分配，直接返回空
	if (base_get_configuration()->prog_max_use_mem < atomic_read(&gmempool_total_memory)) {
		INFO_PRINT("---------------大于指定内存数量\n");
		exit(1);
		return NULL;
	}
	//mempool_index->malloc_size 大小的内存个数+1
	atomic_inc(&(mempool_index->total_count));
	//统计分配的总内存块数(不分大小)
	atomic_inc(&gmem_data_number);

	//链表中没有多余内存，该处分配新内存
#ifdef MEMPOOL_DEBUG_MEM_CHECK
	temp_data = malloc(sizeof(struct mempool_data) + mempool_index->malloc_size + MEM_CHECK_SIZE);
#else
	temp_data = malloc(sizeof(struct mempool_data) + mempool_index->malloc_size);
#endif
	if (!temp_data){
		log_write(LOG_ERROR,"[%s][%s][%d] 内存分配失败 malloc size:%d\n",
				  __FILE__,__func__,__LINE__,mempool_index->malloc_size);
		return NULL;
	}

	temp_data->malloc_size = mempool_index->malloc_size;
	temp_data->mem_number  = atomic_read(&gmem_data_number);
	temp_data->mf_flag     = MEMPOOL_IS_MALLOC;

#ifdef MEMPOOL_DEBUG_MEM_CHECK
	memcpy(temp_data->malloc_data+mempool_index->malloc_size,MEMPOOL_MEM_CHECK,MEM_CHECK_SIZE); //放入内存校验特征
#endif

	MEMPOOL_PRINT("Malloc mem data number :%u, mem size:%d, pthread id:%lu\n",
				  atomic_read(&gmem_data_number),mempool_index->malloc_size,pthread_self());

	//统计内存分配数量
	atomic_add(sizeof(struct mempool_data) + mempool_index->malloc_size, &gmempool_total_memory);

	memset (temp_data->malloc_data,0,temp_data->malloc_size);
	return temp_data->malloc_data;
}

/*
 *函数名称：mempool_data_free
 *函数功能：把数据释放到链表中
 *输入参数：mempool_index      要从该节点分配内存
 *          mp_data       要释放的数据
 *输出参数：none
 *返回值：  void *
*/
static void *mempool_data_free(struct mempool_index *mempool_index,struct mempool_data *mp_data)
{
	MEMPOOL_PRINT("Free mem data number :%lu, mem size:%d,pthread id:%lu\n",
			      mp_data->mem_number,mp_data->malloc_size,pthread_self());

	if (UNLIKELY(MEMPOOL_IS_FREE == mp_data->mf_flag)) {
		DEBUG_PRINT("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~内存重复释放~~~~~~~~~~~~~~~~~~~~~~\n");
//		assert(0);
		return 0;
	}
	//个数+1
	atomic_inc(&(mempool_index->residue_count));

	mp_data->mf_flag = MEMPOOL_IS_FREE;

	pthread_spin_lock(gmempool_spin_lock);
	list_add_tail(&(mp_data->list_node),&(mempool_index->mempool_data_head));
	pthread_spin_unlock(gmempool_spin_lock);

	return 0;
}

/*
 *函数名称：mempool_other_malloc
 *函数功能：分配大于mempool的内存
 *输入参数：malloc_size        要分配内存的大小
 *输出参数：none
 *返回值：  void *
*/
static void *mempool_other_malloc(int malloc_size)
{
	//统计内存分配数量
	atomic_add(sizeof(struct mempool_data) + malloc_size, &gmempool_total_memory);

	struct mempool_data *mp_data = malloc(sizeof(struct mempool_data) + malloc_size);
	if (!mp_data){
		log_write(LOG_ERROR,"[%s][%s][%d] 内存分配失败 malloc size: %d\n",
				  __FILE__,__func__,__LINE__,malloc_size);
		return NULL;
	}

	mp_data->malloc_size = malloc_size;
	mp_data->mem_number  = 0;    //不对大内存做编号，因为是需要进行释放的

	MEMPOOL_PRINT("Other malloc mem size:%d, pthread id:%lu\n", malloc_size,pthread_self());

	memset (mp_data->malloc_data, 0, malloc_size);
	return mp_data->malloc_data;
}

/*
 *函数名称：mempool_other_free
 *函数功能：释放大于mempool的内存
 *输入参数：free_mem      要释放的内存
 *输出参数：none
 *返回值：  none
*/
static void mempool_other_free(void *free_mem)
{
	struct mempool_data *mp_data = (struct mempool_data*)free_mem; 
	atomic_sub(mp_data->malloc_size + sizeof(struct mempool_data), &gmempool_total_memory);

	MEMPOOL_PRINT("Other free mem size:%d, pthread id:%lu\n", mp_data->malloc_size,pthread_self());

	free(free_mem);
	free_mem = NULL;
}

/*
 *函数名称：switch_memdata_size
 *函数功能：选择要分配或者释放内存的大小
 *输入参数：malloc_size       分配内存的大小
 *          mp_data      要释放的内存
 *          malloc_or_free    操作函数（分配函数或者释放函数）
 *输出参数：none
 *返回值：  void *
*/
static inline void *switch_memdata_size(const int malloc_size,struct mempool_data *mp_data,
		                                mf_call_back *malloc_or_free)
{
	assert (malloc_or_free);
	switch(malloc_size){
		case 1 ... MEMPOOL_MALLOC_SIZE_32: 
			return malloc_or_free(&gmempool_index_head[0],mp_data);

		case MEMPOOL_MALLOC_SIZE_32+1 ... MEMPOOL_MALLOC_SIZE_64: 
			return malloc_or_free(&gmempool_index_head[1],mp_data);
			
		case MEMPOOL_MALLOC_SIZE_64+1 ... MEMPOOL_MALLOC_SIZE_128: 
			return malloc_or_free(&gmempool_index_head[2],mp_data);

		case MEMPOOL_MALLOC_SIZE_128+1 ... MEMPOOL_MALLOC_SIZE_256: 
			return malloc_or_free(&gmempool_index_head[3],mp_data);

		case MEMPOOL_MALLOC_SIZE_256+1 ... MEMPOOL_MALLOC_SIZE_512: 
			return malloc_or_free(&gmempool_index_head[4],mp_data);

		case MEMPOOL_MALLOC_SIZE_512+1 ... MEMPOOL_MALLOC_SIZE_1024: 
			return malloc_or_free(&gmempool_index_head[5],mp_data);

		case MEMPOOL_MALLOC_SIZE_1024+1 ... MEMPOOL_MALLOC_SIZE_2048: 
			return malloc_or_free(&gmempool_index_head[6],mp_data);

		case MEMPOOL_MALLOC_SIZE_2048+1 ... MEMPOOL_MALLOC_SIZE_4096: 
			return malloc_or_free(&gmempool_index_head[7],mp_data);

		case MEMPOOL_MALLOC_SIZE_4096+1 ... MEMPOOL_MALLOC_SIZE_8192:
			return malloc_or_free(&gmempool_index_head[8],mp_data);

		case MEMPOOL_MALLOC_SIZE_8192+1 ... MEMPOOL_MALLOC_SIZE_256K:
			return malloc_or_free(&gmempool_index_head[9],mp_data);

		default:
			//mp_data 为空，说明需要分配内存，不为空说明需要释放内存
			if (!mp_data) {
				return mempool_other_malloc(malloc_size);
			} 
			else {
				mempool_other_free(mp_data);
			}
			return NULL;
	}
}

/*
 *函数名称：mempool_display_info
 *函数功能：print mempool info
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void mempool_display_info(int signo)
{
	INFO_PRINT("mempool 分配的总内存数量：%u KB\n",atomic_read(&gmempool_total_memory)/1024);

	int i=0;
	for(i=0;i<MEMPOOL_COUNT+1;i++){  //该处不能 MEMPOOL_COUNT + 1
		INFO_PRINT("memsize[%d] total_count[%u] residue_count[%u]\n",
				   gmempool_index_head[i].malloc_size,
				   atomic_read(&(gmempool_index_head[i].total_count)),
				   atomic_read(&(gmempool_index_head[i].residue_count)));

	}
}

/*
 *函数名称：mempool_memory_info
 *函数功能：test malloc or realloc allocation memory
 *输入参数：memory           test memory pointer
 *输出参数：none
 *返回值：  none
*/
void mempool_memory_info(void *memory,char *mem_name)
{
	if(UNLIKELY(!memory)){
		return ;
	}

	struct mempool_data *mp_data = (struct mempool_data*)(memory - sizeof(struct mempool_data)); 

	INFO_PRINT("mem_size[%d] mem_number[%u]\n",mp_data->malloc_size,mp_data->mem_number);

#ifdef MEMPOOL_DEBUG_MEM_CHECK
	if (memcmp(mp_data->malloc_data+mp_data->malloc_size,MEMPOOL_MEM_CHECK,MEM_CHECK_SIZE)) {
		INFO_PRINT("%s_内存校验失败\n",mem_name);
	}else {
		INFO_PRINT("%s_内存校验成功\n",mem_name);
	}
#endif
}

