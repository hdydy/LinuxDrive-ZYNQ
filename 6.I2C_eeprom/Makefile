#已经编译过的内核源码路径
KERNEL_DIR = /home/uisrc/uisrc-lab-xlnx/sources/kernel

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

#当前路径
CURRENT_DIR = $(shell pwd)

MODULE =  eeprom_24c02r_drv
APP = eeprom_24c02r_app

all : 
#进入并调用内核源码目录中Makefile的规则, 将当前的目录中的源码编译成模块
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) modules 

ifneq ($(APP), )
	$(CROSS_COMPILE)gcc $(APP).c -o $(APP)
endif

clean : 
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) clean
	rm -rf $(APP) *.tmp

#指定编译哪个文件
obj-m += $(MODULE).o