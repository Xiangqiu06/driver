#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

int main(void)
{
    int fd = 0;
    unsigned char data[2] = {0};
    double temp = 0.0;

    fd = open("/dev/i2c-0", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    ioctl(fd, I2C_SLAVE, 0x48);

    while (1)
    {
        write(fd, 0x00, 1);
        read(fd, data, 2);
        temp = ((data[0] << 8 | data[1]) >> 5) * 0.125;
        printf("temp = %.2lf\n", temp);

        sleep(1);
    }

    close(fd);

    return 0;
}