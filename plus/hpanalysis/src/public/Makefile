#
#Makefile for the public
#

CFLAGS += -Wall -O2

OUR_OBJS += hash.o log.o mempool.o pthread_affinity.o public.o stdyeetec.o

# -DNEDBUG                      关闭assert判断
# -DMEMPOOL_DEBUG_PRINT         mempool控制打印输出
# -DMEMPOOL_DEBUG_MEM_CHECK     开启测试内存校验
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
