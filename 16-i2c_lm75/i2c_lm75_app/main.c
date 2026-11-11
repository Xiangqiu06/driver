#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "framebuffer.h"

//SCL 43
//SDA 42




int main(void)
{
    int fd = 0;
    fb_t *pfb = NULL;
    unsigned char tmpbuff[2] = {0};
    unsigned char showscreen[256] = {0};
    double data = 0;

    pfb = framebuff_init("/dev/fb0");
    if (NULL == pfb)
    {
        return -1;
    }

    fd = open("/dev/lm75_misc", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }
    framebuffer_clearscreen(pfb, 255, 255, 255);

    while (1)
    {
        read(fd, tmpbuff, sizeof(tmpbuff));
        printf("temp = %.2lf\n", ((tmpbuff[0] << 8 | tmpbuff[1]) >> 5) * 0.125);//buffer1中的
        data = ((tmpbuff[0] << 8 | tmpbuff[1]) >> 5) * 0.125;
        sprintf(showscreen, "LM75: %10.2lf", data);
        framebuffer_draw_str(pfb, 100, 100, showscreen, 0, 0, 0, 255, 255, 255);
        sleep(1);
    }
    framebuff_deinit(pfb);
    close(fd);

    return 0;
}