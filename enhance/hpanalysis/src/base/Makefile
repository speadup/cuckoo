#
#Makefile for the base
#

CFLAGS += -O2

OUR_OBJS += base.o config.o yt_signal.o

# -DNEDBUG 关闭assert判断
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
