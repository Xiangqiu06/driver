#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>



static ssize_t dht11_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    unsigned char data[5];
    char buf[64];
    int ret;
    int len;

    if (n == 0)
        return 0;
    //mutex_lock_interruptible进程拿不到锁资源会睡眠等待，但是出现信号会让他取消睡眠等待
    if (mutex_lock_interruptible(&dht11_lock))//防止多个进程同时读取dht11数据
        return -ERESTARTSYS;

    ret = dht11_read_data(data);
    if (ret) {
        mutex_unlock(&dht11_lock);
        return ret;
    }

    len = snprintf(buf, sizeof(buf),
                   "temperature=%u C humidity=%u %%\n",
                   data[2], data[0]);

    mutex_unlock(&dht11_lock);
}
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dht11_read,
};

static struct miscdevice dht11_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "dht11",
    .fops = &fops,
};
static int dht11_probe(struct platform_device *pdevice)
{
    int ret = 0;
    
    ret = misc_register(&dht11_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    dht11gpiono = of_get_named_gpio(pdevice->dev.of_node, "gpio-dht11", 0);
    if (keygpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }
    
    ret = devm_gpio_request(dht11_misc.this_device, dht11gpiono, "dht11_platform_drv");
    if (ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_input(dht11gpiono);
    //把驱动中的私有数据保存到 platform_device 设备结构体中，方便后续函数取出来使用
    platform_set_drvdata(pdevice, &dht11_misc);

    pr_info("key_probe success\n");

    return 0;
}

static int dht11_remove(struct platform_device *pdevice)
{
    misc_deregister(&dht11_misc);

    pr_info("key_remove success\n");

    return 0;
}

static struct platform_device_id dht11_id_tables[] = {
    { .name = "dht11"},
    {},
};

static struct of_device_id  dht11_of_match_table[] = {
    {.compatible = "pute,dht11"},
    {},
};

struct platform_driver dht11_drv = {
    .probe = dht11_probe,
    .remove = dht11_remove,
    .driver = {
        .name = "dht11",
        .owner = THIS_MODULE,
        .of_match_table = dht11_of_match_table,
    },
    .id_table = dht11_id_tables,
};

module_platform_driver(dht11_drv);


MODULE_LICENSE("GPL");