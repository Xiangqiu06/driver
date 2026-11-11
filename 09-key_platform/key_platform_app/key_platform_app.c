#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
    int fd = 0;
    int keystat = 0;

    fd = open("/dev/key_misc", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        
        read(fd, &keystat, sizeof(keystat));
        sleep(1);
        printf("keystat = %d\n",keystat);
    }

    close(fd);

    return 0;
}