#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

static int ledgpiono = 0;

static ssize_t led_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    long nret = 0;
    int ledstat = 0;

    nret = copy_from_user(&ledstat, puser, n);
    if (nret) {
        pr_info("copy_from_user failed\n");
        return -1;
    }

    if (1 == ledstat) {
        gpio_set_value(ledgpiono, 0);
    } else if (0 == ledstat) {
        gpio_set_value(ledgpiono, 1);
    }

    return 0;
}   

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = led_write,
};

static struct miscdevice led_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "led_misc",
    .fops = &fops,
};

static int __init led_misc_init(void)
{
    int ret = 0;
    struct device_node *plednode = NULL;

    ret = misc_register(&led_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    //在设备中查找对应节点
    plednode = of_find_node_by_path("/myled0");
    if (NULL == plednode) {
        pr_info("of_find_node_by_path failed\n");
        return -1;
    }

    //找到gpio对应的编号
    ledgpiono = of_get_named_gpio(plednode, "gpio-myled0", 0);
    if (ledgpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }

    //申请使用gpio
    ret = devm_gpio_request(led_misc.this_device, ledgpiono, "led_misc_drv");
    if (-1 == ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_output(ledgpiono, 1);

    return 0;
}

static void __exit led_misc_exit(void)
{
    misc_deregister(&led_misc);

    return;
}

module_init(led_misc_init);
module_exit(led_misc_exit);

MODULE_LICENSE("GPL");