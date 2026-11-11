#将demo_drv.o加入内核模块编译中，虽然demo_drv.o没有，但根据隐式原则会将demo_drv.c编译生成demo_drv.o 
obj-m += $(MODULE_DRVNAME).o 

#内核路径变量定义并赋值
kerdir := /home/linux/drive/kernel/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek

#内核模块所在路径
curdir := $(shell pwd)

all:
	make -C $(kerdir) M=$(curdir) modules 
	cp $(MODULE_DRVNAME).ko ~/nfs/rootfs

.PHONY:
clean:
	make -C $(kerdir) M=$(curdir) modules clean
distclean:
	rm ~/nfs/rootfs/$(MODULE_DRVNAME).ko
	make -C $(kerdir) M=$(curdir) modules clean
	
