#
#Makefile for the main
#

CFLAGS += -Wall -O2

OUR_OBJS += main.o 

# -DNEDBUG 关闭assert判断
DEBUG_FLAG += -DNEDBUG

MK_PATH=./
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
