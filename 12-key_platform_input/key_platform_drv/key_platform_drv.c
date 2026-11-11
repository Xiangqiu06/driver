#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/of_irq.h>
#include <linux/workqueue.h>
#include <linux/input.h>



static int irqno = 0;
struct input_dev *pkey_dev = NULL;

static void mytasklet_func(unsigned long data)
{
    input_event(pkey_dev, EV_KEY, KEY_0, 1);
    input_sync(pkey_dev);
    input_event(pkey_dev, EV_KEY, KEY_0, 0);
    input_sync(pkey_dev);

    return;
}

static DECLARE_TASKLET(mytasklet, mytasklet_func, 0);

static irqreturn_t key_isr(int irqno, void *pdev)
{
    tasklet_schedule(&mytasklet);

    return IRQ_HANDLED;
}

static int key_probe(struct platform_device *pdevice)
{
    int ret = 0;
    pkey_dev = input_allocate_device();
    if (NULL == pkey_dev) {
        pr_info("input_allocate_device failed\n");
        return -1;
    }
    input_set_capability(pkey_dev,EV_KEY,KEY_0);

    ret = input_register_device(pkey_dev);
    if (ret) {
        pr_info("input_register_device failed\n");
        return -1;
    }

    irqno = of_irq_get(pdevice->dev.of_node, 0);
    if (irqno < 0) {
        pr_info("of_irq_get failed\n");
        return -1;
    }
    ret = devm_request_irq(&pkey_dev->dev, irqno, key_isr, IRQF_TRIGGER_FALLING, "key_platform_input_drv", NULL);
    if (ret) {
        pr_info("devm_request_irq failed\n");
        return -1;
    }
    return 0;
}            

static int key_remove(struct platform_device *pdevice)
{
    input_unregister_device(pkey_dev);
    input_free_device(pkey_dev);
    

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