#ifndef _PUBLIC_DEBUG_H_
#define _PUBLIC_DEBUG_H_

////////////////////////////////////////MEMPOOL_DEBUG///////////////////////////////////////////

#ifdef MEMPOOL_DEBUG_PRINT
#define MEMPOOL_PRINT(fmt, args...) fprintf(stdout, "[%s][%s][%d] "fmt, \
		__FILE__, __FUNCTION__, __LINE__, ##args)
#else
#define MEMPOOL_PRINT(fmt, args...)
#endif  //MEMPOOL_DEBUG_PRINT

#endif  //_PUBLIC_DEBUG_H_

