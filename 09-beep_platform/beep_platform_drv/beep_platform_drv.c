#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

static int beepgpiono = 0;

static ssize_t beep_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
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

    return n;
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

static int beep_probe(struct platform_device *pdevice)
{
    int ret = 0;
    
    ret = misc_register(&beep_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    beepgpiono = of_get_named_gpio(pdevice->dev.of_node, "gpio-mybeep0", 0);
    if (beepgpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }
    
    ret = devm_gpio_request(beep_misc.this_device, beepgpiono, "beep_platform_drv");
    if (ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_output(beepgpiono, 1);

    pr_info("beep_probe success\n");

    return 0;
}            

static int beep_remove(struct platform_device *pdevice)
{
    misc_deregister(&beep_misc);

    pr_info("beep_remove success\n");

    return 0;
}

static struct platform_device_id beep_id_tables[] = {
    { .name = "mybeep"},
    {},
};

static struct of_device_id  beep_of_match_table[] = {
    {.compatible = "pute,mybeep"},
    {},
};

struct platform_driver beep_drv = {
    .probe = beep_probe,
    .remove = beep_remove,
    .driver = {
        .name = "mybeep",
        .owner = THIS_MODULE,
        .of_match_table = beep_of_match_table,
    },
    .id_table = beep_id_tables,
};

module_platform_driver(beep_drv);

#if 0
static int __init beep_platform_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&beep_drv);
    if (ret) {
        pr_info("platform_driver_register failed\n");
        return -1;
    }

    return 0;
}

static void __exit beep_platform_exit(void)
{
    platform_driver_unregister(&beep_drv);

    return;
}

module_init(beep_platform_init);
module_exit(beep_platform_exit);
#endif

MODULE_LICENSE("GPL");