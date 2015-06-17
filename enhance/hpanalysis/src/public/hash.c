/***********************************************************
 *文件名：hash.c
 *创建人：小菜_默
 *日  期：2013年11月23日
 *描  述：从nids拷贝的，在原来的基础上添加了ip6项
 *
 *修改人：xxxxx
 *日  期：xxxx年xx月xx日
 *描  述：
 *************************************************************/

#include "hash.h"
#include "stdyeetec.h"

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define HASHSIZE 44
static unsigned char xor[HASHSIZE];
static unsigned char perm[HASHSIZE];

static void getrnd ()
{
	struct timeval s;
	unsigned *ptr;
	int fd = open ("/dev/urandom", O_RDONLY);
	if (fd > 0){
	    read (fd, xor, HASHSIZE);
	    read (fd, perm, HASHSIZE);
	    close (fd);

	    return;
	}
	
	gettimeofday (&s, 0);
	srand (s.tv_usec);
	ptr = (u_int *) xor;
	*ptr = rand ();
	*(ptr + 1) = rand ();
	*(ptr + 2) = rand ();
	ptr = (u_int *) perm;
	*ptr = rand ();
	*(ptr + 1) = rand ();
	*(ptr + 2) = rand ();
}

/*
 *函数名称：init_hash
 *函数功能：初始化hash
 *输入参数：none
 *输出参数：none
 *返回值：  none
*/
void init_hash ()
{
	INFO_PRINT("Public->Hash模块初始化\n");

	int i, n, j;
	int p[HASHSIZE];

	getrnd ();
	for (i = 0; i < HASHSIZE; i++){
		p[i] = i;
	}

	for (i = 0; i < HASHSIZE; i++){
	    n = perm[i] % (HASHSIZE - i);
	    perm[i] = p[n];
	    for (j = 0; j < (HASHSIZE-1) - n; j++)
	  	p[n + j] = p[n + j + 1];
	}
}

/*
 *函数名称：mkhash
 *函数功能：根据输入的ip和port返回hash值
 *输入参数：ip_src             ipv4 source ip
 *          ip6_src            ipv6 source ip
 *          sport              source port
 *          ip_dest            ipv4 dest ip
 *          ip6_dest           ipv6 dest ip
 *          dport              dest port
 *输出参数：none
 *返回值：  hash value
*/
unsigned mkhash (unsigned ip_src,unsigned *ip6_src,unsigned short sport,
		         unsigned ip_dest,unsigned *ip6_dest,unsigned short dport)
{
	int i;
	unsigned res = 0;
	unsigned char data[HASHSIZE];
	unsigned *stupid_strict_aliasing_warnings=(unsigned*)data;

	*stupid_strict_aliasing_warnings = ip_src;
	*(unsigned *) (data + 4)  = ip_dest;

	//add ip6 src ip and dest ip
/*	*(unsigned *) (data + 4)  = ip6_src[0];
	*(unsigned *) (data + 8)  = ip6_src[1];
	*(unsigned *) (data + 12) = ip6_src[2];
	*(unsigned *) (data + 16) = ip6_src[3];

	*(unsigned *) (data + 20) = ip6_dest[0];
	*(unsigned *) (data + 24) = ip6_dest[1];
	*(unsigned *) (data + 28) = ip6_dest[2];
	*(unsigned *) (data + 32) = ip6_dest[3];
*/
	*(unsigned *) (data + 8)  = 0;
	*(unsigned *) (data + 12)  = 0;
	*(unsigned *) (data + 16) = 0;
	*(unsigned *) (data + 20) = 0;

	*(unsigned *) (data + 24) = 0;
	*(unsigned *) (data + 28) = 0;
	*(unsigned *) (data + 32) = 0;
	*(unsigned *) (data + 36) = 0;

	*(unsigned short *) (data + 40) = sport;
	*(unsigned short *) (data + 42) = dport;
	
	for (i = 0; i < HASHSIZE; i++){
		res = ( (res << 8) + (data[perm[i]] ^ xor[i])) % 0xff100f;
	}
	
	return res;
}

