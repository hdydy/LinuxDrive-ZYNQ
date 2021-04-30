#已经编译过的内核源码路径
KERNEL_DIR = /home/uisrc/uisrc-lab-xlnx/sources/kernel

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

#当前路径
CURRENT_DIR = $(shell pwd)

APP1 = can_send
APP2 = can_receive

all :
	$(CROSS_COMPILE)gcc $(APP1).c -o $(APP1)
	$(CROSS_COMPILE)gcc $(APP2).c -o $(APP2)

clean : 
	rm -rf $(APP1) $(APP2)

