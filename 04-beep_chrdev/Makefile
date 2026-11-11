MODULENAME := beep_chrdev
MODULE_APPNAME := $(MODULENAME)_app
MODULE_DRVNAME := $(MODULENAME)_drv

export MODULE_APPNAME MODULE_DRVNAME

all:
	make -C $(MODULE_APPNAME) 
	make -C $(MODULE_DRVNAME) 
	
.PHONY:
clean:
	make -C $(MODULE_APPNAME) clean 
	make -C $(MODULE_DRVNAME) clean
distclean:
	make -C $(MODULE_APPNAME) distclean 
	make -C $(MODULE_DRVNAME) distclean