#
#Makefile for the feature_manager
#

CFLAGS += -Wall -O2

OUR_OBJS += feature_manager.o feature_manager_deal.o feature_manager_place.o

#DB_INFO_PRINT
# -DNEDBUG 关闭assert判断
# -DDB_DEBUG_PRINT

DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
