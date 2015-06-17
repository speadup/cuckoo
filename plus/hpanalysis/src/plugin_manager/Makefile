#
#Makefile for the plugin_manager
#

CFLAGS += -Wall -O2

OUR_OBJS += plugin_manager.o

# -DNEDBUG 关闭assert判断
# -DPLUGIN_MANAGER_DEBUG
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o
