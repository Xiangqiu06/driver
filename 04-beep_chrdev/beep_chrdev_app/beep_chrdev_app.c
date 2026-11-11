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

    fd = open("/dev/mybeep0", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        beepstat = 1;
        write(fd, &beepstat, sizeof(beepstat));
        read(fd, &curstat, sizeof(curstat));
        printf("beep curstat is %s\n", (1 == curstat) ? "BEEP_ON" : "BEEP_OFF");
        sleep(1);

        beepstat = 0;
        write(fd, &beepstat, sizeof(beepstat));
        read(fd, &curstat, sizeof(curstat));
        printf("beep curstat is %s\n", (1 == curstat) ? "BEEP_ON" : "BEEP_OFF");
        sleep(1);
    }

    close(fd);

    return 0;
}