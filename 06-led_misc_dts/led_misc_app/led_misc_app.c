#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    int fd = 0;
    int ledstat = 0;
    int curstat = 0;

    fd = open("/dev/led_misc", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        ledstat = 1;
        write(fd, &ledstat, sizeof(ledstat));
        sleep(1);

        ledstat = 0;
        write(fd, &ledstat, sizeof(ledstat));
        sleep(1);
    }

    close(fd);

    return 0;
}