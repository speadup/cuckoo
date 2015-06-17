/***********************************************************
*文件名：plugin_public.c
*创建人：韩涛
*日  期：2013年10月21日
*描  述：公共源文件
*
*修改人：xxxxx
*日  期：xxxx年xx月xx日
*描  述：
*
************************************************************
*/

#define _GNU_SOURCE
#include "stdyeetec.h"
#include "plugin_encode.h"
#include "plugin_public.h"
#include "feature_manager_public_struct.h"
#include "list.h"
#include "plugin_debug.h"
#include "string.h"


/*
*函数名称：plugin_mark
*函数功能：mark匹配函数
*调用函数：无
*输入函数：deal_data：待匹配的客户端数据 data_length：数据长度
*输出参数：无
*返回值：  成功YT_SUCCESSFUL 失败YT_FAILED
*/
static int plugin_mark(char *deal_data, int data_length, struct list_head *mark_feature)
{
	char *location = NULL;
	struct list_head *list_head_mark_tmp = NULL;
	struct plugin_feature_mark *plugin_feature_mark_tmp = NULL;

	//循环遍历mark链
	list_for_each(list_head_mark_tmp, mark_feature){
		plugin_feature_mark_tmp = (struct plugin_feature_mark*)list_head_mark_tmp;
		//根据匹配方式进行匹配,
		switch (plugin_feature_mark_tmp->flag) {
			
			case 0:		/* 偏移类型: 需要进行部分的偏移之后进行匹配 */
				//该方式要保证feature不为空
				if(!(plugin_feature_mark_tmp->feature)){
					return YT_FAILED;
				}
			
				/*因为考虑到大小写问题所以没用memcmp，应该用的,记得改*/
				if(!strcasestr(deal_data, plugin_feature_mark_tmp->feature)){ 
				
					return YT_FAILED;		
				}
				
				break;
			
			case 1:		/* 结构化类型: 在特定的地方进行匹配 */
				location = strstr(deal_data, plugin_feature_mark_tmp->offset_or_head);	
				if(!location){
					return YT_FAILED;
				}
				
				if(memcmp(location + strlen(plugin_feature_mark_tmp->offset_or_head), plugin_feature_mark_tmp->feature, strlen(plugin_feature_mark_tmp->feature))){
					return YT_FAILED;;
				}

				break;
				
			case 2:		
				//该方式有两个字段特征因此没有考虑为空的情况
				if(plugin_feature_mark_tmp->offset_or_head){
					if(!strstr(deal_data, plugin_feature_mark_tmp->offset_or_head)){
						return YT_FAILED;	
					}
				}
				
				if(plugin_feature_mark_tmp->feature){		
					if(!strstr(deal_data, plugin_feature_mark_tmp->feature)){
						return YT_FAILED;
					}
				}

				break;
			
			default:
				break;
		}
	}

	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_feature_mark
*函数功能：匹配协议
*输入函数：deal_data：待匹配的数据 data_length：数据长度
*输出参数：无
*返回值：  成功返回链表头 失败返回NULL
*/
struct plugin_feature *plugin_feature_mark(char *deal_data, int data_length, struct plugin_feature *plugin_feature_head)
{
	if(!deal_data || !data_length){
		return NULL;
	}
	
	struct list_head *list_head_tmp = NULL;
	struct plugin_feature *plugin_feature_tmp = NULL;

	//循环遍历特征码链
	list_for_each(list_head_tmp, &(plugin_feature_head->list_node)){
		plugin_feature_tmp = (struct plugin_feature*)list_head_tmp;
	
		if(plugin_mark(deal_data, data_length, &(plugin_feature_tmp->pmark_list_node))){
			continue;
		}
		else{
			return plugin_feature_tmp;
		}
	}

	return NULL;
}

/*
*函数名称：plugin_feature_extract
*函数功能：遍历截取特征码调用相应处理函数
*输入函数：deal_data：待处理的服务端的数据 data_len：数据长度 plugin_fe_list_node:截取特征码链表头
*输出参数：plugin_dst:存储结构体
*返回值：  0表示成功 -1表示失败
*/
int plugin_feature_extract(char *deal_data, int data_len, struct list_head *plugin_fe_list_node, void *plugin_dst)
{
	struct list_head *list_head_extract_tmp = NULL;
	struct plugin_feature_extract *plugin_fe = NULL;
	
	if(!deal_data || (data_len < 5) || !plugin_dst){
		return YT_FAILED;
	}	
	
	//循环遍历截取特征码，回调处理函数
	list_for_each(list_head_extract_tmp, plugin_fe_list_node){
		plugin_fe = (struct plugin_feature_extract*)list_head_extract_tmp;
		
		if(!(plugin_fe->excb)){
			log_write(LOG_ERROR, "plugin_extract excb is NULL\n");
			continue;
		}
		
		(plugin_fe->excb)(deal_data, plugin_dst, plugin_fe);
	}
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_extract_type_deal
*函数功能：根据类型截取
*输入参数：deal_data：待截取数据 dst_len：目的长度 plugin_fe:截取特征码
*输出参数：plugin_dst:存储
*返回值：  0成功 -1失败
*/
int plugin_extract_type_deal(char *deal_data, char *dst, int dst_len, struct plugin_feature_extract *plugin_fe)
{
	char *head = NULL;
	int length = 0;
	
	switch (plugin_fe->flag) {
		case 0:		/* 偏移类型:	需要进行部分的偏移之后进行截取 */
			if(!(plugin_fe->offset_or_head) || !(plugin_fe->offset_or_tail)){
				log_write(LOG_ERROR, "plugin_extract_type_deal case 0 偏移量错误\n");
				return YT_FAILED;
			}
			
			head = deal_data + atoi(plugin_fe->offset_or_head);
			length = atoi(plugin_fe->offset_or_tail);
			memcpy(dst, head, MIN(dst_len, length));

			break;

		case 1:		/* 结构化类型: 这种方式保留，暂时不处理 */
			
			break;
		
		case 2:		/* 直接截取 */
			if(plugin_data_extract(deal_data, plugin_fe->offset_or_head, 
					plugin_fe->offset_or_tail, dst, dst_len)){
				return YT_FAILED;
			}
			break;

		default:
			break;
	}
	
	return YT_SUCCESSFUL;
}

/*
*函数名称：plugin_data_extract
*函数功能：截取字符串
*输入参数：deal_data：待截取数据 
		   extract_head：截取头
		   extract_tail：截取尾
		   dst_length:截取长度	
*输出参数：dst：存放处理后数据
*返回值：  0成功 -1失败
*/
int plugin_data_extract(char *deal_data, char *extract_head, char *extract_tail, char *dst, int dst_length)
{
	if(!deal_data || !extract_head || !dst){
		return YT_FAILED;
	}

	char *p_head = NULL;
	char *p_tail = NULL;

	p_head = strcasestr(deal_data, extract_head);
	if(UNLIKELY(!p_head)){
		return YT_FAILED;
	}
	else{
		if(!extract_tail){
			//如果尾巴为空，就将整个串拷贝
			memcpy(dst, p_head + strlen(extract_head), MIN(dst_length, strlen(p_head)));
			return YT_SUCCESSFUL;
		}

		//如果尾巴不为空，找到了就拷贝，没找到那就返回错误
		p_head += strlen(extract_head);
		p_tail = strstr(p_head, extract_tail);

		if(UNLIKELY(!p_tail)){
			log_write(LOG_ERROR, "extract_head find extract_tail not find\n");
			return YT_FAILED;
			//原先的做法是如果没有找到就拷贝head之后所有数据，但可能是数据不全
			//等下次拼起来之后再找一次。
			//memcpy(dst, p_head, MIN(dst_length, strlen(p_head)));
		}
		else{
			memcpy(dst, p_head, MIN(dst_length, p_tail - p_head));
		}
		
		return YT_SUCCESSFUL;
	}
	
	return YT_FAILED;
}

/*
*函数名称：plugin_address_deal
*函数功能：处理地址去除<>
*输入参数：deal_data：待处理数据 dst_len：目的长度
*输出参数：dst：存放位置
*返回值：  0 成功 -1 失败
*/
int plugin_address_deal(char *deal_data, char *dst, int dst_len)
{
	char temp[COMMON_LENGTH_64];
	char *p_msg_body = deal_data;
	char *pstart = NULL;
	char *pend = NULL;
	int i = 0;
	
	do{
		
		memset(temp, 0, COMMON_LENGTH_64);	
		
		pstart = strstr(p_msg_body, "<");
		pend = strstr(p_msg_body, ">");
		
		if(pstart && pend){
			pstart++;
			memcpy(temp, pstart, pend - pstart);
		
			//规避第一次
			if(i>0){	
				strcat(dst, ",");
			}
			
			//大小超限问题
			strncat(dst, temp, strlen(temp));
			p_msg_body = pend + 1; 
		}
		else{
			//不存在尖括号，多个地址也不处理了，若处理则考虑空格分隔
			//函数只处理了有尖括号跟没尖括号的 那种两者都有的处理暂时不考虑
			if(i == 0){
				memcpy(dst, deal_data, strlen(deal_data));
			}

			return YT_SUCCESSFUL;
		}
		
		i++;

	}while(1);

	return YT_SUCCESSFUL;	
}


/*
*函数名称：convert_encode
*函数功能：转码
*输入参数：dst：待处理数据  dst_len:目的长度 encode:转码类型
*输出参数：dst：存放位置
*返回值：  0 成功 -1 失败
*/

void convert_encode(char *str, int len, enum TEXT_ENCODE encode, char *extra)
{
	//char *g_temp_buffer;
	
	if (!str || (str[0] == 0)) 
		return;
/*
	if (encode == ENCODE_CUSTOM_UTF16|| encode == ENCODE_URL_CUSTOM_UTF16) {
		if (extra[0]) {
			get_data_between_marks(extra, "UTF16PREFIX:", "|", g_temp_buffer, LENGTH_TEMP_BUFF);
		}
	}
*/
	switch (encode) {
	case ENCODE_CUSTOM_UTF16:
		convert_custom_utf16_to_utf8(str);
		break;
	case ENCODE_GB18030:
		convert_gb18030_to_utf8(str, len);
		break;
	case ENCODE_URL_CUSTOM_UTF16:
		break;
	case ENCODE_URL_GB18030:
		convert_urlcharset_to_utf8(str, "GB18030");
		break;
	case ENCODE_URL_UTF16:
		break;
	case ENCODE_URL_UTF8:
		convert_urlcharset_to_utf8(str, "UTF-8");
		break;
	case ENCODE_UTF16:
		break;
	case ENCODE_UTF8:
		break;
	case ENCODE_BASE64:
		decode_base64(str, len);
		break;
	default:
		break;
	}
}

/*
*函数名称：convert_encodes
*函数功能：转码
*输入参数：dst：待处理数据  dst_len:目的长度 encode:转码类型
*输出参数：dst：存放位置
*返回值：  0 成功 -1 失败
*/
//例如encode="123"，表示需要依次进行1、2、3类型的code转码
void convert_encodes(char *dst, int dst_len, char *encode, char *extra)
{
	 int i;
     for(i=0;i<strlen(encode);i++){       
		convert_encode(dst, dst_len, encode[i] - '0', extra);
	 }
}

/*
*函数名称：plugin_write_file
*函数功能：写文件
*调用函数：无
*输入参数：put_type：写方式 data：写数据  data_length：数据长度  file_path_name：全路径
*输出参数：无
*返回值：0 成功 -1失败
*/
int plugin_write_file(char *put_type, char *data, int data_length, FILE *file_fp)
{
PLUGIN_PRINT("+++++++++++++++++++++++++++++++\n");	
	if (file_fp){
		if(!fwrite(data, data_length, 1, file_fp)){
			log_write(LOG_ERROR, "plugin_write_file fwrite() error\n");
			return YT_FAILED;
		}
		return YT_SUCCESSFUL;
	}
	
	return YT_FAILED;
}


/*
*函数名称：memstr
*函数功能：在内存块中查找字符串
*调用函数：无
*输入参数：haystack：待搜索区 needle：被搜索的字符串  size：搜索区长度
*输出参数：无
*返回值：!NULL 成功 NULL 失败
*/
char *memstr(char *haystack, char *needle, int size)
{
    char *p;
    char needlesize = strlen(needle);

    for (p = haystack; p <= (haystack-needlesize+size); p++)
    {
        if (memcmp(p, needle, needlesize) == 0)
            return p; /* found */
    }
    return NULL;
}
