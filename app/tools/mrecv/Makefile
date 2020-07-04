#!/bin/bash
############################################################################
#
############################################################################
#源文件，自动找所有.c和.cpp文件，并将目标定义为同名.o文件
SOURCE  := $(wildcard *.c) $(wildcard *.cpp)
OBJS    := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
  
#目标文件名，输入任意你想要的执行文件名
TARGET  := mrecv.bin
ROOT_DIR := $(shell pwd)

############################################################################  
# 编译参数
############################################################################  
# 编译器
CC := arm-hisiv500-linux-gcc
LD := arm-hisiv500-linux-ld

# 编译宏
DEFINES :=

# 头文件路径
INCLUDE := -I$(ROOT_DIR)/include  

# 链接库
LIBS := -lpthread 

# 链接选项
LDFLAGS :=

# CFLAGS汇总处理
CFLAGS := -g -Wall -O3 $(DEFINES) $(INCLUDE) $(LIBS)

############################################################################  
# 下面的基本上不需要做任何改动了
############################################################################
.PHONY : everything clean rebuild

everything : $(TARGET)
all : $(TARGET)

rebuild: clean everything

clean:
	echo "Removing linked and compiled files......"
	rm -fr $(TARGET)
	rm -fr *.o

$(TARGET) : $(OBJS)
	echo $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS) 
############################################################################