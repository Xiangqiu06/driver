#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

static int ledgpiono = 0;

static ssize_t led_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
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

    return n;
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

static int led_probe(struct platform_device *pdevice)
{
    int ret = 0;
    
    ret = misc_register(&led_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    ledgpiono = of_get_named_gpio(pdevice->dev.of_node, "gpio-myled0", 0);
    if (ledgpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }
    
    ret = devm_gpio_request(led_misc.this_device, ledgpiono, "led_platform_drv");
    if (ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_output(ledgpiono, 1);

    pr_info("led_probe success\n");

    return 0;
}            

static int led_remove(struct platform_device *pdevice)
{
    misc_deregister(&led_misc);

    pr_info("led_remove success\n");

    return 0;
}

static struct platform_device_id led_id_tables[] = {
    { .name = "myled"},
    {},
};

static struct of_device_id  led_of_match_table[] = {
    {.compatible = "pute,myled"},
    {},
};

struct platform_driver led_drv = {
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "myled",
        .owner = THIS_MODULE,
        .of_match_table = led_of_match_table,
    },
    .id_table = led_id_tables,
};

module_platform_driver(led_drv);

#if 0
static int __init led_platform_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&led_drv);
    if (ret) {
        pr_info("platform_driver_register failed\n");
        return -1;
    }

    return 0;
}

static void __exit led_platform_exit(void)
{
    platform_driver_unregister(&led_drv);

    return;
}

module_init(led_platform_init);
module_exit(led_platform_exit);
#endif

MODULE_LICENSE("GPL");