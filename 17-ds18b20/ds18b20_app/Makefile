OBJ:=framebuffer

OBJS+=framebuffer.c 
OBJS+=main.c 
OBJS+=font_8x16.c 

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