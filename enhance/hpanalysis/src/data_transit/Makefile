#
#Makefile for the data_transit
#

CFLAGS += -O2

OUR_OBJS += data_transit.o

# -DNEDBUG 关闭assert判断
# -DDATA_TRANSIT_MEM_DEBUG 调试内存
# -DDATA_TRANSIT_DEBUG
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
