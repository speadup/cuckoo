#
#Makefile for the net
#

#包含模块的子模块目录
include ethernet/Makefile
include ipv4/Makefile
include ipv6/Makefile
include tcp/Makefile
include udp/Makefile

VPATH=ethernet:ipv4:ipv6:tcp:udp

#配置模块的编译选项
CFLAGS += -Wall -O2

#要编译的.o模块
OUR_OBJS += net.o

# -DNEDBUG                关闭assert判断
# -DETH_DEBUG_PRINT       ethernet debug print
# -DIPv4_DEBUG_PRINT      ip debug print
# -DTCP_DEBUG_PRINT       tcp debug print
# -DTCP_FUNCTION_RUN_TIME 打开tcp模块的函数运行时间统计
DEBUG_FLAG += -DNEDBUG

MK_PATH=../
include $(MK_PATH)mk/hpanalysis.mk

clean:
	rm -f *.o

