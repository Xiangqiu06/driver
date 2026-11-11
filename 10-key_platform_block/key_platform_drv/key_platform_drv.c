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

extern void msleep(unsigned int msecs);

static int keygpiono = 0;
static wait_queue_head_t wq;
static int condition = 0;
static int irqno = 0;
static struct work_struct workqueue;

//工作队列中断底半部执行函数
static void key_workqueue_func(struct work_struct *work)
{
    //work_queue可以进行延时，因为work_queue是进程上下文，等到执行工作队列中的任务时，中断就已经执行完了
    msleep(10);
    pr_info("key pressed\n");

    return;
}
//task_let中断底半部执行函数
static void key_func(unsigned long data)
{
    msleep(10);//task_let中不能进行延时，因为task_let是中断上下文，中断只能快进快出，否则程序会崩溃
    pr_info("key pressed\n");

    return;
}
static DECLARE_TASKLET(mytasklet, key_func, 0);

static irqreturn_t key_handler(int irqno, void *pdev)
{   
    //中断顶半部执行
    /* 唤醒等待的key_read等待队列 */
    condition = 1;
    wake_up_interruptible(&wq);
    //中断底半部执行
//    schedule_work(&workqueue);
  tasklet_schedule(&mytasklet);
    
    return IRQ_HANDLED;
}

static ssize_t key_read (struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
    int keystat = 0;

    condition = 0;
    wait_event_interruptible(wq,condition);

    keystat = gpio_get_value(keygpiono);
    nret = copy_to_user(puser, &keystat, sizeof(keystat));
    if(nret)
    {
        pr_info("copy_to_user failed\n");
        return -1;
    }
    return sizeof(keystat);

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
    INIT_WORK(&workqueue, key_workqueue_func);
    init_waitqueue_head(&wq);

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

    irqno = of_irq_get(pdevice->dev.of_node, 0);
    if (irqno < 0) {
        pr_info("of_irq_get failed\n");
        return -1;
    }

    ret = devm_request_irq(key_misc.this_device, irqno, key_handler, IRQF_TRIGGER_FALLING, "key_platform_drv", NULL);
    if (ret) {
        pr_info("request_irq failed\n");
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