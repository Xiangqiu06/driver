#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <linux/input.h>

int main(void)
{
    int fd = 0;
    struct input_event env;

    fd = open("/dev/input/event1", O_RDWR);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    while (1)
    {
        read(fd, &env, sizeof(env));
        printf("env.type = %#x\n", env.type);
        printf("env.code = %#x\n", env.code);
        printf("env.value = %#x\n", env.value);
    }

    close(fd);

    return 0;
}