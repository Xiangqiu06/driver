OBJ:=lm75_userspace

OBJS:=lm75_userspace.c 

CC:=arm-linux-gnueabihf-gcc 

$(OBJ):$(OBJS)
	$(CC) $^ -o $@
	cp $(OBJ) ~/nfs/rootfs

.PHONY:
clean:
	rm $(OBJ)
distclean:
	rm $(OBJ)
	rm ~/nfs/rootfs/$(OBJ)