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

static int beepgpiono = 0;

static ssize_t beep_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    long nret = 0;
    int beepstat = 0;

    nret = copy_from_user(&beepstat, puser, n);
    if (nret) {
        pr_info("copy_from_user failed\n");
        return -1;
    }

    if (1 == beepstat) {
        gpio_set_value(beepgpiono, 0);
    } else if (0 == beepstat) {
        gpio_set_value(beepgpiono, 1);
    }

    return 0;
}   

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = beep_write,
};

static struct miscdevice beep_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "beep_misc",
    .fops = &fops,
};

static int __init beep_misc_init(void)
{
    int ret = 0;
    struct device_node *pbeepnode = NULL;

    ret = misc_register(&beep_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    //在设备中查找对应节点
    pbeepnode = of_find_node_by_path("/mybeep0");
    if (NULL == pbeepnode) {
        pr_info("of_find_node_by_path failed\n");
        return -1;
    }

    //找到gpio对应的编号
    beepgpiono = of_get_named_gpio(pbeepnode, "gpio-mybeep0", 0);
    if (beepgpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }

    //申请使用gpio
    ret = devm_gpio_request(beep_misc.this_device, beepgpiono, "beep_misc_drv");
    if (-1 == ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_output(beepgpiono, 1);

    return 0;
}

static void __exit beep_misc_exit(void)
{
    misc_deregister(&beep_misc);

    return;
}

module_init(beep_misc_init);
module_exit(beep_misc_exit);

MODULE_LICENSE("GPL");