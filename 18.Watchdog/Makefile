#已经编译过的内核源码路径
KERNEL_DIR = /home/uisrc/uisrc-lab-xlnx/sources/kernel

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

#当前路径
CURRENT_DIR = $(shell pwd)

APP = watchdog_test
#APP = watchdog

all : 
#进入并调用内核源码目录中Makefile的规则, 将当前的目录中的源码编译成模块
ifneq ($(APP), )
	$(CROSS_COMPILE)gcc $(APP).c -o  $(APP)
endif

clean : 
	rm $(APP)

#指定编译哪个文件
obj-m += $(MODULE).o
