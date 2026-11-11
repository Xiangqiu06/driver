#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(void)
{
    int fd = 0;
    ssize_t nret = 0;
    unsigned char data[4] = {0};

    fd = open("/dev/dht11_misc", O_RDWR);
    if (-1 == fd) 
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        nret = read(fd, data, sizeof(data));
        if (nret < 0) {
            break;
        }

        printf("temp:%d.%d  hum:%d.%d\n", data[2], data[3], data[0], data[1]);
        sleep(2);
    }
    
    close(fd);

    return 0;
}