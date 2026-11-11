#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

//ADC 7引脚、MQ传感器

#define MQ2_ADC_FILE  "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"

/* 比"干净空气基准值"高出这么多就报警，先设 300，按实测再调 */
#define ALARM_OFFSET  800

/* 读一次 ADC，返回 0~4095 的原始值，失败返回 -1 */
static int mq2_read_raw(void)
{
    int fd = 0;
    ssize_t nret = 0;
    char buf[32] = {0};

    fd = open(MQ2_ADC_FILE, O_RDONLY);
    if (-1 == fd)
    {
        perror("fail to open");
        return -1;
    }

    nret = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (nret <= 0)
        return -1;

    return atoi(buf);
}

int main(void)
{
    int i = 0;
    int raw = 0;
    int base = 0;
    int sum = 0;

    /* 第一步：上电先采样 10 次求平均，作为"当前空气"的基准值 */
    printf("calibrating, keep sensor in clean air ...\n");

    for (i = 0; i < 10; i++)
    {
        raw = mq2_read_raw();
        if (raw < 0)
            return -1;

        sum += raw;
        sleep(1);
    }

    base = sum / 10;
    printf("baseline = %d\n", base);

    /* 第二步：以后每次读数都跟基准值比 */
    while (1)
    {
        raw = mq2_read_raw();
        if (raw < 0)
            break;

        if (raw > base + ALARM_OFFSET)
            printf("raw:%4d  base:%4d  >>> smoke detected\n", raw, base);
        else
            printf("raw:%4d  base:%4d  normal\n", raw, base);

        sleep(1);
    }

    return 0;
}