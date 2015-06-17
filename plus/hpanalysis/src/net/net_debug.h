#ifndef _NET_DEBUG_H_
#define _NET_DEBUG_H_

//////////////////////////////////////////Etherne Debug///////////////////////////////////////
#ifdef ETH_DEBUG_PRINT
//direction: 1、"SMAC" 2、"DMAC"
#define ETH_PRINT_MAC(direction,paddr) fprintf(stdout, "net [%s][%d:line] \
%s->%02X:%02X:%02X:%02X:%02X:%02X\n",__FILE__,__LINE__,direction,\
((unsigned char *)paddr)[0],((unsigned char *)paddr)[1],\
((unsigned char *)paddr)[2],((unsigned char *)paddr)[3],\
((unsigned char *)paddr)[4],((unsigned char *)paddr)[5])

#else

#define ETH_PRINT_MAC(direction,paddr)

#endif  //ETH_DEBUG_PRINT

///////////////////////////////////////Ipv4 Debug///////////////////////////////////////////
#ifdef IPv4_DEBUG_PRINT
//direction:1、"SIP" 2、"DIP"
#define IPv4_PRINT_IP(direction,paddr) fprintf(stdout, "net [%s][%d line] \
%s->%03d.%03d.%03d.%03d\n",__FILE__,__LINE__,direction,\
((unsigned char *)paddr)[0],((unsigned char *)paddr)[1],\
((unsigned char *)paddr)[2],((unsigned char *)paddr)[3])

#else

#define IPv4_PRINT_IP(direction,paddr)

#endif //IPv4_DEBUG_PRINT

//////////////////////////////////////Ipv6 Debug////////////////////////////////////////////




/////////////////////////////////////Tcp Debug//////////////////////////////////////////////
#ifdef TCP_DEBUG_PRINT

//direction:1、"SPORT" 2、"DPORT"
#define TCP_PRINT_PORT(direction,port) fprintf(stdout,"net [%s][%d line] %s->%d \n",\
		__FILE__,__LINE__,direction,port)

#define TCP_PRINT(fmt, args...) fprintf(stdout, "[%s][%s][%d] "fmt, \
		__FILE__, __FUNCTION__, __LINE__, ##args)
#else

#define TCP_PRINT_PORT(direction,port)
#define TCP_PRINT(fmt, args...)

#endif  //TCP_DEBUG_PRINT
#endif  //_NET_DEBUG_H_

#ifdef TCP_FUNCTION_RUN_TIME

#define TIME_BEGIN(id) \
struct timeval tv_begin_id,tv_end_id;\
do {\
	gettimeofday(&tv_begin_id,NULL);\
}while(0)


#define TIME_END(id) \
do {\
	gettimeofday(&tv_end_id,NULL);\
	INFO_PRINT ("use time:%ld us\n",tv_end_id.tv_usec - tv_begin_id.tv_usec);\
}while(0)

#else

#define TIME_BEGIN
#define TIME_END

#endif




