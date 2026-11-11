#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>

static int keygpiono = 0;


static ssize_t key_read (struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
    int keystat = 1;
    keystat = gpio_get_value(keygpiono);
    nret = copy_to_user(puser, &keystat, sizeof(keystat));
    if(nret)
    {
        pr_info("copy_to_user failed\n");
        return -1;
    }
    return n;

}


static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = key_read,
};

static struct miscdevice key_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "key_misc",
    .fops = &fops,
};

static int key_probe(struct platform_device *pdevice)
{
    int ret = 0;
    
    ret = misc_register(&key_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    keygpiono = of_get_named_gpio(pdevice->dev.of_node, "gpio-mykey0", 0);
    if (keygpiono < 0) {
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }
    
    ret = devm_gpio_request(key_misc.this_device, keygpiono, "key_platform_drv");
    if (ret) {
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_input(keygpiono);
    //gpio_direction_output(keygpiono, 1);//引脚，初始电平

    pr_info("key_probe success\n");

    return 0;
}            

static int key_remove(struct platform_device *pdevice)
{
    misc_deregister(&key_misc);

    pr_info("key_remove success\n");

    return 0;
}

static struct platform_device_id key_id_tables[] = {
    { .name = "mykey"},
    {},
};

static struct of_device_id  key_of_match_table[] = {
    {.compatible = "pute,mykey"},
    {},
};

struct platform_driver key_drv = {
    .probe = key_probe,
    .remove = key_remove,
    .driver = {
        .name = "mykey",
        .owner = THIS_MODULE,
        .of_match_table = key_of_match_table,
    },
    .id_table = key_id_tables,
};

module_platform_driver(key_drv);

#if 0
static int __init key_platform_init(void)
{
    int ret = 0;

    ret = platform_driver_register(&key_drv);
    if (ret) {
        pr_info("platform_driver_register failed\n");
        return -1;
    }

    return 0;
}

static void __exit key_platform_exit(void)
{
    platform_driver_unregister(&key_drv);

    return;
}

module_init(key_platform_init);
module_exit(key_platform_exit);
#endif

MODULE_LICENSE("GPL");