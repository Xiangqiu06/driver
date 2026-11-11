#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "framebuffer.h"
//CS 17

int main(void)
{
    fb_t *pfb = NULL;
    int fd = 0;
    short acc[3] = {0};
    char tmpbuff[256] = {0};
    ssize_t nret = 0;

    pfb = framebuff_init("/dev/fb0");
    if (NULL == pfb)
    {
        return -1;
    }
    
    fd = open("/dev/adxl345_misc", O_RDWR);
    if (-1 == fd) 
    {
        perror("fail to open");
        return -1;
    }
    framebuffer_clearscreen(pfb, 255, 255, 255);

    while (1)
    {
        nret = read(fd, acc, sizeof(acc));
        printf("x:%05d  y:%05d  z:%05d\n", acc[0], acc[1], acc[2]);
        sprintf(tmpbuff, "X:%05d  Y:%05d  Z:%05d",acc[0], acc[1], acc[2]);
        framebuffer_draw_str(pfb, 100, 100, tmpbuff, 0, 0, 0, 255, 255, 255);
        usleep(1000);
    }
    framebuff_deinit(pfb);
    close(fd);

    return 0;
}