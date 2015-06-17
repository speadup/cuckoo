#
#Makefile for the feature_manager
#

CFLAGS += -Wall -O2

OUR_OBJS += sqlite_database.o

#DB_INFO_PRINT
# -DNEDBUG 关闭assert判断
# -DDB_DEBUG_PRINT

DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
