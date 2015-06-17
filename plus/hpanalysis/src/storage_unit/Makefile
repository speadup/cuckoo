#
#Makefile for the storage_unit
#

CFLAGS += -O2

OUR_OBJS += storage_unit.o storage_unit_timer.o storage_unit_database.o

# -DNEDBUG 关闭assert判断
#-DPATH_DEBUG_PRINT 
#-DNUM_DEBUG_PRINT

DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
