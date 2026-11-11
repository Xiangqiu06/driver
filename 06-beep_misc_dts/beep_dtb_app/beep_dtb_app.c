#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    int fd = 0;
    int beepstat = 0;
    int curstat = 0;

    fd = open("/dev/beep_misc", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        beepstat = 1;
        write(fd, &beepstat, sizeof(beepstat));
        sleep(1);

        beepstat = 0;
        write(fd, &beepstat, sizeof(beepstat));
        sleep(1);
    }

    close(fd);

    return 0;
}