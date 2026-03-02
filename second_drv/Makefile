ARCH := x86_64
CC := /usr/bin/gcc
CROSS_COMPILE :=

# 1. 定义内核源码目录
KERNEL_DIR ?= /lib/modules/$(shell uname -r)/build

# 2. 定义模块所在目录
PWD := $(shell pwd)

# 3. 编译目标：调用内核源码的Makefile进行模块编译
all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

# 4. 内核模块编译目标：obj-m 表示要构建为可加载模块
obj-m := second_drv.o  # 必须与.c文件名一致

# 5. 清理目标：删除编译生成的中间文件和模块文件
clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean