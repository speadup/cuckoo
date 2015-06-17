#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>
#include <ctype.h>
#include "plugin_public.h"
#include "stdyeetec.h"

//find string needle from haystack ,case ignored .haystack and needle must be ASCII string
//return the pointer point to needle found in haystack,NULL if not found 
char *strcasestr(const char *haystack,const char *needle)
{

	const char *p_haystack = NULL;
	const char *p_needle = NULL;
	const char *p = NULL;


	if(haystack == NULL || needle == NULL)
		return NULL;

	for(p_haystack=haystack;*p_haystack != 0;p_haystack++)
	{
		for(p_needle=needle,p=p_haystack;*p_needle!=0&&*p!=0;p_needle++,p++)
		{
			if(*p_needle == *p
				||(*p_needle>64&&*p_needle<91&&*p_needle==(*p-32))
				||(*p_needle>96&&*p_needle<123&&*p_needle==(*p+32)))
				continue;
			else
				break;
		}
		if(*p_needle == 0)//reach end of needle
			return (char *)p_haystack;
	}
	return NULL;
}


int str2int_array(const char *str, int *array, const int array_size)
{
	int count = 0;
	if( str == NULL || array == NULL )
	{
		return count;
	}
	while(*str != '\0')
	{
		if(!isdigit(*str))
		{
			str++;
		}
		else
		{
			if(count < array_size)
			{
				array[count++] = atoi(str);
				while(isdigit(*str) )
					str++;
			}
			else
			{
				break;
			}
		}


	}
	return  count;
}



//由于进行iconv操作shir时，会对ppin和ppout的值进行转换，同时注意所指空间也被替换，因此请保证实际的传入值是动态分配的
int  code_convert(char *from_charset, char *to_charset, char **ppin, size_t src_len, char **ppout, size_t des_len )
{
	if(!ppin || !ppout || !*ppin || !*ppout)
		return -1;

	if(!from_charset || !to_charset){
		if(*ppout != *ppin)
			memcpy(*ppout, *ppin, src_len);
		return -1;
	}

	iconv_t cd = iconv_open(to_charset, from_charset);
	if( (iconv_t)(-1) == cd ){
		if(*ppout != *ppin)
			memcpy(*ppout, *ppin, src_len);
		return -1;
	}

	char *pin = *ppin;
	char *pout = *ppout;

	int flag = iconv(cd, ppin, &src_len, ppout, &des_len);

	//hello,由于iconv前后的*ppin值和*ppout值被改变，在使用mempool的情况下，应当进行变更，以避免出现野指针等问题
	//实际上，iconv将传入的数据的值放置到新的空间中，而传入的指针地址表示该位置，原来空间的值被新的编码字符值替代
	*ppin = pin;
	*ppout = pout;

	iconv_close(cd);

	return flag;
}

void get_element_by_flag(char *buf, char *to, char *start_flag, char *end_flag)
{       
	char *pstart = NULL, *pend = NULL;
	if(!buf || !to || !start_flag || !end_flag)
		return;
	pstart = strstr(buf, start_flag);
	if(pstart){
		pstart = pstart + strlen(start_flag);
		if(pstart){
			pend = strstr(pstart, end_flag); 
			if(pend){
				memcpy(to, pstart, pend - pstart);
			}
		}
	}
}

void reset_http_esc(char *psrc, int length, char *pneedle, char c_reset)
{
	char *pthis=NULL;
	char *pclear=psrc+length-1;
	int success_flag =0;
	int i,j = 0;
	if(!psrc || !pneedle || length<3)
		return;
	i = strlen(pneedle);
	while(length>3){
		pthis = strstr(psrc, pneedle);
		if(pthis){
			*pthis = c_reset;
			memmove(pthis+1, pthis+i, length-(pthis-psrc)-i);
			length -= (i-1);
			success_flag++;
		}else{
			break;
		}
	}	
	if(success_flag)
	{
		for(j=0; j<success_flag; j++)
		{
			while((--i))
			{
				*pclear-- ='\0';
			}
			i = strlen(pneedle);
		}
	}
	return;
}


static const char table64[]=
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int find64(char c)
{
	int i=0;
	for(i=0;i<sizeof(table64);i++)
		if(c==table64[i])return 1;
	return 0;
}

/*
int base64_encode(const char *inp, int insize, char **outptr)
{
char ibuf[3];
char obuf[4];
int i;
int inputparts;
char *output;
char *base64data;
char *indata = (char *)inp;
*outptr = NULL; 
if(0 == insize)
insize = strlen(indata);
base64data = output = (char*)pool_malloc(insize*4/3+4);
if(NULL == output)
return 0;
while(insize > 0) {
for (i = inputparts = 0; i < 3; i++) {
if(insize > 0) {
inputparts++;
ibuf[i] = *indata;
indata++;
insize--;
}
else
ibuf[i] = 0;
}
obuf [0] = (ibuf [0] & 0xFC) >> 2;
obuf [1] = ((ibuf [0] & 0x03) << 4) | ((ibuf [1] & 0xF0) >> 4);
obuf [2] = ((ibuf [1] & 0x0F) << 2) | ((ibuf [2] & 0xC0) >> 6);
obuf [3] = ibuf [2] & 0x3F;
switch(inputparts) {
case 1: 
snprintf(output, 5, "%c%c==",
table64[obuf[0]],
table64[obuf[1]]);
break;
case 2: 
snprintf(output, 5, "%c%c%c=",
table64[obuf[0]],
table64[obuf[1]],
table64[obuf[2]]);
break;
default:
snprintf(output, 5, "%c%c%c%c",
table64[obuf[0]],
table64[obuf[1]],
table64[obuf[2]],
table64[obuf[3]] );
break;
}
output += 4;
}
*output=0;
*outptr = base64data; 
return strlen(base64data); 
}
*/

/*_____________________________________________Lastest base64__________________________________________________________*/
//---------------------------------------------------------------------------
//  4bit binary to char 0-F
char Hex2Chr( char n ) 
{
	n &= 0xF;
	if ( n < 10 )
		return ( char )( n + '0' );
	else
		return ( char )( n - 10 + 'A' );
}
//---------------------------------------------------------------------------
//  char 0-F to 4bit binary

char Chr2Hex( char c ) 
{
	if ( c >= 'a' && c <= 'z' )  //  it's toupper
		c = c - 'a' + 'A';
	if ( c >= '0' && c <= '9' )
		return ( int )( c - '0' );
	else if ( c >= 'A' && c <= 'F' )
		return ( int )( c - 'A' + 10 );
	else
		return -1; 
}
//---------------------------------------------------------------------------
//  Base64 code table
//  0-63 : A-Z(25) a-z(51), 0-9(61), +(62), /(63)

char  Base2Chr( char n ) 
{
	n &= 0x3F;
	if ( n < 26 )
		return ( char )( n + 'A' );
	else if ( n < 52 )
		return ( char )( n - 26 + 'a' );
	else if ( n < 62 )
		return ( char )( n - 52 + '0' );
	else if ( n == 62 )
		return '+';
	else
		return '/';
}
//---------------------------------------------------------------------------

char Chr2Base( char c )
{
	if ( c >= 'A' && c <= 'Z' )
		return ( char )( c - 'A' );
	else if ( c >= 'a' && c <= 'z' )
		return ( char )( c - 'a' + 26 );
	else if ( c >= '0' && c <= '9' )
		return ( char )( c - '0' + 52 );
	else if ( c == '+' )
		return 62;
	else if ( c == '/' )
		return 63;
	else
		return 64;  //  无效字符
}
//---------------------------------------------------------------------------
//  aLen 为 aSrc 的大小， aDest 所指的缓冲区必须至少为 aLen 的 3 倍！！！
//  返回 aDest 的长度
/* hello
int QPEncode( char * const aDest, const char * aSrc, int aLen )
{
char * p = aDest;
int    i = 0;
while ( i++ < aLen )
{
*p++ = '=';
*p++ = Hex2Chr( *aSrc >> 4 );
*p++ = Hex2Chr( *aSrc++ );
}
*p = 0;  //  aDest is an ASCIIZ string
return ( p - aDest );  //  exclude the end of zero
}
//---------------------------------------------------------------------------
//  aDest 所指的缓冲区必须至少为 aSrc 长度的 1/3 ！！！
//  返回 aDest 的长度

int QPDecode( char * const aDest, const char * aSrc )
{
char * p = aDest;
int             n = strlen( aSrc );
char   ch, cl;
while ( *aSrc )  //  aSrc is an ASCIIZ string
{
if ( ( *aSrc == '=' ) && ( n - 2 > 0 ) )
{
ch = Chr2Hex( aSrc[1] );
cl = Chr2Hex( aSrc[2] );
if ( ( ch == ( char )-1 ) || ( cl == ( char )-1 ) )
*p++ = *aSrc++;
else
{
*p++ = ( ch << 4 ) | cl;
aSrc += 3;
}
}
else
*p++ = *aSrc++;
}
return ( p - aDest );
}
*/

//---------------------------------------------------------------------------
//  aLen 为 aSrc 的长度， aDest 所指的缓冲区必须至少为 aLen 的 1.33 倍！！！
//  返回 aDest 的长度a

int Base64Encode( char * const aDest, const char * aSrc, int aLen )
{
	char        * p = aDest;
	int           i;
	char t=0;
	for ( i = 0; i < aLen; i++ )
	{
		switch ( i % 3 )
		{
		case 0 :
			*p++ = Base2Chr( *aSrc >> 2 );
			t = ( *aSrc++ << 4 ) & 0x3F;
			break;
		case 1 :
			*p++ = Base2Chr( t | ( *aSrc >> 4 ) );
			t = ( *aSrc++ << 2 ) & 0x3F;
			break;
		case 2 :
			*p++ = Base2Chr( t | ( *aSrc >> 6 ) );
			*p++ = Base2Chr( *aSrc++ );
			break;
		}
	}
	if ( aLen % 3 != 0 )
	{
		*p++ = Base2Chr( t );
		if ( aLen % 3 == 1 )
			*p++ = '=';
		*p++ = '=';
	}
	*p = 0;  //  aDest is an ASCIIZ string
	return ( p - aDest );  //  exclude the end of zero
}

int Base64Decode( char * const aDest, const char *aSrc )
{
	char * p = aDest;
	int             i;
	int     n = strlen( aSrc );
	char   c;
	char   t=0;
	for ( i = 0; i < n; i++ )
	{
		if ( *aSrc == '=' )
			break;
		do {
			if ( *aSrc )
				c = Chr2Base( *aSrc++ );
			else
				c = 65;  //  字符串结束
		} while ( c == 64 );  //  跳过无效字符，如回车等
		if ( c == 65 )
			break;
		switch ( i % 4 )
		{
		case 0 :
			t = c << 2;
			break;
		case 1 :
			*p++ = ( char )( t | ( c >> 4 ) );
			t = ( char )( c << 4 );
			break;
		case 2 :
			*p++ = ( char )( t | ( c >> 2 ) );
			t = ( char )( c << 6 );
			break;
		case 3 :
			*p++ = ( char )( t | c );
			break;
		}
	}

	return ( p - aDest );
}
//---------------------------------------------------------------------------
/* hello
void qp(char sour,char first,char second)
{
if(sour>127){
first=sour>>4;
second=sour&15;
if(first>9)first+=55;
else first+=48;
if(second>9)second+=55;
else second+=48;
}
printf("%c%c%c\n",'=',first,second);
}

void uqp(char sour,char first,char second)
{
if(first>=65)first-=55;
else first-=48;
if(second>=65)second-=55;
else second-=48;
sour=0;
sour=first<<4;
sour|=second;
}
*/
void decode_base64(char *str, int len)
{
	if(!str || len < 0) 
		return;

	char *tempdst = (char*)malloc(len);
	Base64Decode(tempdst, str);
	memset(str, 0, len);
	memcpy(str, tempdst, MIN(len, strlen(tempdst)));
	
	free(tempdst);
}


void base64_decode(char *str, char *dst)
{
	int rte = Base64Decode(dst, str);
}

int EncodeQuoted(const char* pSrc, char* pDst, int nSrcLen, int nMaxLineLen)
{
	int nDstLen=0;
	int nLineLen=0;
	int i=0;
	for (i = 0; i < nSrcLen; i++, pSrc++){
		if ((*pSrc >= '!') && (*pSrc <= '~') && (*pSrc != '=')){
			*pDst++ = (char)*pSrc;
			nDstLen++;
			nLineLen++;
		}else{
			sprintf(pDst, "=%02X", *pSrc);
			pDst += 3;
			nDstLen += 3;
			nLineLen += 3;
		}
		if (nLineLen >= nMaxLineLen - 3){
			sprintf(pDst, "=\r\n");
			pDst += 3;
			nDstLen += 3;
			nLineLen = 0;
		}
	}
	*pDst = '\0';
	return nDstLen;
}

int DecodeQuoted(const char* pSrc, char *pDst, int nSrcLen)
{
	int nDstLen=0;
	int i=0;
	while (i < nSrcLen){
		if (strncmp(pSrc, "=\r\n", 3) == 0){
			pSrc += 3;
			i += 3;
		}else{
			if (*pSrc == '='){
				sscanf(pSrc, "=%02X", (int *)pDst);
				pDst = (char *)pDst;
				pDst++;
				pSrc += 3;
				i += 3;
			}else{
				*pDst++ = (char)*pSrc++;
				i++;
			}
			nDstLen++;
		}
	}
	*pDst = '\0'; 
	return nDstLen;
}

int is_utf8(const char *data)
{
	char *start;
	if ( data == NULL )
		return -1;
	start = strchr( data, '%' );
	if ( start == NULL )
		return -1;
	if ( ( *(start+1) == 'E' ) && ( *(start+3) == '%' ) && ( *(start+6) == '%' ) )
		return 1;
	else
		return 0;
	return -1;
}
/*
int unicode_to_utf8(uint16_t *in, int insize, uint8_t **out)
{
int i = 0;
int outsize = 0;
int charscount = 0;
uint8_t *result = NULL;
uint8_t *tmp = NULL;

charscount = insize / sizeof(uint16_t);
result = (uint8_t *)malloc(charscount * 3 + 1);
memset(result, 0, charscount * 3 + 1);
tmp = result;

for (i = 0; i < charscount; i++)
{
uint16_t unicode = in[i];

if (unicode >= 0x0000 && unicode <= 0x007f)
{
*tmp = (uint8_t)unicode;
tmp += 1;
outsize += 1;
}
else if (unicode >= 0x0080 && unicode <= 0x07ff)
{
*tmp = 0xc0 | (unicode >> 6);
tmp += 1;
*tmp = 0x80 | (unicode & (0xff >> 2));
tmp += 1;
outsize += 2;
}
else if (unicode >= 0x0800 && unicode <= 0xffff)
{
*tmp = 0xe0 | (unicode >> 12);
tmp += 1;
*tmp = 0x80 | (unicode >> 6 & 0x00ff);
tmp += 1;
*tmp = 0x80 | (unicode & (0xff >> 2));
tmp += 1;
outsize += 3;
}

}

*tmp = '\0';
*out = result;
return 0;
}
*/
/*urldecode*/
int url_decode( char *dst, const char *src, int src_len )
{
	char c1,c2;
	int i = 0;
	int j = 0;
	char *p_dst = dst;
	const char *p_src = src;

	if ( ( dst == NULL ) || ( src == NULL )  || (src_len == 0))
		return -1;

	for ( i = 0, j = 0; i < src_len; ){
		if ( *(p_src+i) == '%' ){
			c1 = tolower( *(p_src+i+1) );
			c2 = tolower( *(p_src+i+2) );
			if ( !isxdigit(c1) || !isxdigit(c2) ){
				/*If not urlencoded data then copy it*/
				*(p_dst+j) = *(p_src+i);
				*(p_dst+j+1) = *(p_src+i+1);
				*(p_dst+j+2) = *(p_src+i+2);
				j += 3;
				i += 3;
			}else{
				if ( c1 <= '9' )
					c1 = c1 - '0';
				else
					c1 = c1 - 'a' + 10;
				if ( c2 <= '9' )
					c2 = c2 - '0';
				else
					c2 = c2 - 'a' + 10;
				*(p_dst+j) = 16 * c1 + c2;
				j++;
				i += 3;
			}
		}else if ( *(p_src+i) == '+' ){
			*(p_dst+j) = ' ';
			j++;
			i++;
		}else{
			*(p_dst+j) = *(p_src+i);
			j++;
			i++;
		}
	}

	*(p_dst+j) = '\0';
	return j;
}

/*
char *url_decode3(char *psrc, char pdst_addr[])
{
if(!psrc)
return NULL;

int i, j=0, ilen=strlen(psrc);
char ptmp[2];
//hello old is int
int num;

for( i = 0; i < ilen; i++ ){
memset( ptmp, '\0', 2 );
if( '+' == psrc[i]){
pdst_addr[j++] = ' ';
continue;
}else if( '%' != psrc[i]){
pdst_addr[j++] = psrc[i];
continue;
}
ptmp[0] = psrc[++i];
ptmp[1] = psrc[++i];
sscanf( ptmp, "%x", &num );
sprintf(ptmp, "%c", num );
pdst_addr[j++] = ptmp[0];
}
pdst_addr[j] = '\0';
return pdst_addr;
}
*/

/////////////////////////////////////////////////////////////////
char *search_value_by_flag(char **pptmp, char *pstart, char *pend)
{
	char *pchar = NULL, *pvalue = NULL;
	if(!pptmp || !pstart || !pend || !*pstart || !*pend || !*pptmp || !**pptmp)
		return NULL;

	pchar = strstr(*pptmp, pstart);
	if(pchar){
		pchar += strlen(pstart);
		pvalue = pchar;
		pchar = strstr(pchar, pend);
		if(pchar)
			*pchar++ = '\0';
		*pptmp = pchar;
	}
	return pvalue;
}

//从给定地址开始，查找第一个出现的\r或者\n，并设置为\0
void zero_first_ctrl(char *pchar)
{
	if(!pchar)
		return;
	while('\0'!=*pchar){
		if('\r'==*pchar || '\n'==*pchar){
			*pchar = '\0';
			break;
		}
		pchar++;	
	}

}

void convert_urlcharset_to_utf8(char *paddr, char *charset)
{
	//转换之后的内容仍然存放在原来的地址处，由于URI的空间明显会比目标格式信息空间大，因此不存在空间溢出问题
	if(!paddr || !charset || '\0'==paddr[0])
		return;

	int ilen = strlen(paddr);
	char *ptmp = (char *)malloc(ilen+1);
	int ret = is_utf8(paddr);

	if(ptmp){
		memset(ptmp, 0, ilen+1);
		url_decode((char *)ptmp, (char *)paddr, ilen);

		int len_ptmp = strlen(ptmp);
		// if utf-8 ,then do nothing
		if(1 == ret){
			memset(paddr, 0, strlen(paddr));
			memcpy(paddr, ptmp, strlen(ptmp));
		}else{
			char *putf8 = malloc(len_ptmp*2+1);
			memset(putf8, 0, len_ptmp*2+1);
			code_convert(charset, "UTF-8", &ptmp, len_ptmp, &putf8, len_ptmp*2);

			int dest_len = strlen(putf8);
			if(dest_len > ilen){
				dest_len = ilen;
			}

			//problem 如果是gb2312的空间来源，写数据会产生截断，空间应当由外部产生而截断，或者干脆用malloc申请到的

			memset(paddr, 0, dest_len+1);
			memcpy(paddr, putf8, dest_len);

			free(putf8);
			putf8 = NULL;
		}

		free(ptmp);
		ptmp = NULL;
	}
}


void convert_custom_utf16_to_utf8(char *pmsg)
{
	char *prefix = NULL;

	if (!pmsg) 
		return;

	/* 寻找utf16的分隔符号 */
	do {
		if (strstr(pmsg, "%u")) {
			prefix = "%u";
			break;
		}
		if (strstr(pmsg, "&#x")) {
			prefix = "&#x";
			break;
		}
		if (strstr(pmsg, "%U")) {
			prefix = "%U";
			break;
		}
		if (strstr(pmsg, "%U+")) {
			prefix = "%U+";
			break;
		}
	} while(0);

	if (!prefix)
		return;

	int prefix_len = strlen(prefix);
	char *utf8msg = pmsg;
	char *putf8cur = utf8msg;
	int i;int for_loop;
	unsigned char utf16char[2];
	for (i = 0; i < strlen(pmsg); ) { 
		if (strncmp(pmsg+i, prefix, prefix_len) == 0) {	// match prefix
			/* convert UTF16 string to byte */
			memset(utf16char, 0, 2);
			unsigned char *strmac = (unsigned char*)pmsg+i+prefix_len; 
			for (for_loop = 0; for_loop < 4; ++for_loop) {
				strmac[for_loop] = toupper(strmac[for_loop]);
				utf16char[for_loop/2] = utf16char[for_loop/2] | ( strmac[for_loop]>0x40?((strmac[for_loop]-0x37)<<(for_loop%2?0:4)):((strmac[for_loop]-0x30)<<(for_loop%2?0:4)));
			}
			/* convert utf16 to utf8 */
			putf8cur[0] = (0xE0 | ((utf16char[0] & 0xF0) >> 4));    
			putf8cur[1] = (0x80 | ((utf16char[0] & 0x0F) << 2)) + ((utf16char[1] & 0xC0) >> 6);    
			putf8cur[2] = (0x80 | (utf16char[1] & 0x3F));  
			putf8cur+=3; i += (4+prefix_len);
		} else {
			ZYSTRNCPY(putf8cur, pmsg+i, 1);
			putf8cur++;++i;
		}
	}
	putf8cur[0] = 0;
}

void convert_gb18030_to_utf8(char *str, int len)
{
	if(!str || len < 0) 
		return;

	char *tempsrc = (char*)malloc(len+1);
	char *tempdst = (char*)malloc(len*2+1);
	if(!tempsrc || !tempdst) 
		return;

	memcpy(tempsrc, str, len);

	code_convert("GB18030", "UTF-8", &tempsrc, len, &tempdst, len*2);

	memcpy(str, tempdst, len);
	free(tempsrc);
	free(tempdst);
}

void mail_conver_head_acount(char *p_mail_acount)
{
	if(!p_mail_acount || p_mail_acount[0] == 0)
		return;

	//向前寻找账号开始
	char *pcharmid = strchr(p_mail_acount, '@');
	if(!pcharmid)
		return;

	char *p_email_start = pcharmid;
	//向前寻找账号开始
	while('\0'!=*p_email_start && '<'!=*p_email_start && '>'!=*p_email_start && ','!=*p_email_start && ';'!=*p_email_start && '\"'!=*p_email_start && '\''!=*p_email_start && ' '!=*p_email_start && '\t'!=*p_email_start && '\r'!=*p_email_start && '\n'!=*p_email_start && '='!=*p_email_start && '&'!=*p_email_start && p_mail_acount!=p_email_start+1)
		p_email_start--;
	p_email_start+=1;

	char *p_email_end = pcharmid;
	//向后寻找账号结束
	while('\0'!=*p_email_end && '<'!=*p_email_end && '>'!=*p_email_end && ','!=*p_email_end && ';'!=*p_email_end && '\"'!=*p_email_end && '\''!=*p_email_end && ' '!=*p_email_end && '\t'!=*p_email_end && '\r'!=*p_email_end && '\n'!=*p_email_end && '='!=*p_email_end && '&'!=*p_email_end)
		p_email_end++;
	*p_email_end=0;
	p_email_end++;
	char temp[COMMON_LENGTH_128+1];
	strncpy(temp, p_email_start, MIN(COMMON_LENGTH_128, p_email_end-p_email_start));
	strncpy(p_mail_acount, temp, COMMON_LENGTH_128);
}
