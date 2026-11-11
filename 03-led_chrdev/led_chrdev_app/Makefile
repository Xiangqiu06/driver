CC := arm-linux-gnueabihf-gcc

$(MODULE_APPNAME):$(MODULE_APPNAME).c 
	$(CC) $^ -o $@
	cp $(MODULE_APPNAME) ~/nfs/rootfs

.PHONY:
clean:
	rm $(MODULE_APPNAME)
distclean:
	rm ~/nfs/rootfs/$(MODULE_APPNAME)
	rm $(MODULE_APPNAME)