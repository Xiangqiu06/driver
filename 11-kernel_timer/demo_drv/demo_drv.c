#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/timer.h>


static void timeout(unsigned long data)
{
    pr_info("timeout ok!\n");

    return;
}

static DEFINE_TIMER(mtimer, timeout, 0, 0);

static int __init kernel_timer_init(void)
{
    init_timer(&mtimer);
    add_timer(&mtimer);
    mod_timer(&mtimer, jiffies + msecs_to_jiffies(200));//jiffies表示的是当前时间，当前时间超过1s
    pr_info("add timer success\n");
    return 0;
}

static void __exit kernel_timer_exit(void)
{
    del_timer(&mtimer);

    pr_info("del timer success\n");

    return;
}

module_init(kernel_timer_init);

module_exit(kernel_timer_exit);


MODULE_LICENSE("GPL");