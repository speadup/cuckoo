#ifndef _PACKET_GET_DEBUG_H_
#define _PACKET_GET_DEBUG_H_

////////////////////////////////////////CONFIG_DEBUG///////////////////////////////////////////
#ifdef CONFIG_DEBUG_PRINT

#define CONFIG_PRINT(fmt, args...) fprintf(stderr, "[%s][%s][%d] "fmt, \
		__FILE__, __FUNCTION__, __LINE__, ##args)
#else

#define CONFIG_PRINT(fmt, args...)

#endif

#endif  //_PACKET_GET_DEBUG_H_
