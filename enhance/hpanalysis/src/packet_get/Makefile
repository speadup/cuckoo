#
#Makefile for the packet_get
#

CFLAGS += -Wall -O2

OUR_OBJS += packet_get.o

# -DNEDBUG 关闭assert判断
# -DCONFIG_DEBUG_PRINT
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
