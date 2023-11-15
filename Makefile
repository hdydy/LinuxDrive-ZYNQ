KERNEL_DIR = /home/uisrc/uisrc-lab-xlnx/sources/kernel

export ARCH=arm64
export CROSS_COMPILE=aarch64-linux-gnu-

CURRENT_DIR = $(shell pwd)

MODULE = gpio_reg_drv
APP = gpio_reg_app

all : 
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) modules 
ifneq ($(APP), )
	$(CROSS_COMPILE)g++ $(APP).cpp -o $(APP)
endif

clean : 
	make -C $(KERNEL_DIR) M=$(CURRENT_DIR) clean
	rm $(APP)

obj-m += $(MODULE).o