#************************************************************
# *文件名：hpanalysis.mk
# *创建人：小菜_默
# *日  期：2013年11月22日
# *描  述：程序编译部分，生成.o
# *
# *修改人：xxxxx
# *日  期：xxxx年xx月xx日
# *描  述：
# ************************************************************

include $(MK_PATH)mk/pmake.mk

#获取所有文件目录确保能包含.h
INCLUDE_DIRS := $(shell find $(MK_PATH) -type d |sed "s/.*/-I &/g" |grep -v svn |grep -v mk |grep -v arch)\
				-I $(MK_PATH)sqlite_include

all_objs:$(OUR_OBJS)

#生成.o
$(OUR_OBJS):%.o:%.c 
	$(CC) $(CPU_DIGITS) -Wall -c $(CFLAGS) $(DEBUG_FLAG) $(INCLUDE_DIRS) $<

