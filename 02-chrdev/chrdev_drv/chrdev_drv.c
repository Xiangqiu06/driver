#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>

static int __init chrdev_init(void)
{
    pr_info("chrdev_init success!\n");

    return 0;
}

static void __exit chrdev_exit(void)
{
    pr_info("chrdev_exit success!\n");

    return;
}

//驱动程序的入口
module_init(chrdev_init);

//驱动程序的出口
module_exit(chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pute");