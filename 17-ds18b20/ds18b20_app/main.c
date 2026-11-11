#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "framebuffer.h"

//

int main(void)
{
    fb_t *pfb = NULL;
    int fd_ds18b20 = 0;
    short temp = 0;
    double tempvalue = 0;
    char tmpbuff[256] = {0};
    ssize_t nret = 0;

    pfb = framebuff_init("/dev/fb0");
    if (NULL == pfb)
    {
        return -1;
    }

    fd_ds18b20 = open("/dev/ds18b20_misc", O_RDWR);
    if (-1 == fd_ds18b20)
    {
        perror("fail to open");
        return -1;
    }

    framebuffer_clearscreen(pfb, 255, 255, 255);

    while (1)
    {
        nret = read(fd_ds18b20, &temp, sizeof(temp));
        if (nret != sizeof(temp)) 
        {
            perror("fail to read ds18b20");
            break;
        }

        tempvalue = temp * 0.0625;
        sprintf(tmpbuff, "DS18B20: %10.2lf", tempvalue);

        framebuffer_draw_str(pfb, 100, 100, tmpbuff, 0, 0, 0, 255, 255, 255);

        sleep(1);
    }

    framebuff_deinit(pfb);

    return 0;
}