#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int fd = 0;
    unsigned char tmpbuff[2] = {0};

    fd = open("/dev/lm75_misc", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        read(fd, tmpbuff, sizeof(tmpbuff));
        printf("temp = %.2lf\n", ((tmpbuff[0] << 8 | tmpbuff[1]) >> 5) * 0.125);//buffer1中的
        sleep(1);
    }
    
    close(fd);

    return 0;
}