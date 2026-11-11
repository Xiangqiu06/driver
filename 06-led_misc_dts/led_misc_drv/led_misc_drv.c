#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

static void __iomem *pmux_ctl_addr = NULL;
static void __iomem *pmux_pad_addr = NULL;
static void __iomem *pgpio_dir_addr = NULL;
static void __iomem *pgpio_dat_addr = NULL;

static ssize_t led_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    int value = 0;
    long nret = 0;
    int ledstat = 0;

    nret = copy_from_user(&ledstat, puser, n);
    if (nret) {
        pr_info("copy_from_user failed\n");
        return -1;
    }

    if (1 == ledstat) {
        value = readl(pgpio_dat_addr);
        value &= ~(0x1 << 3);
        writel(value, pgpio_dat_addr);
    } else if (0 == ledstat) {
        value = readl(pgpio_dat_addr);
        value |= (0x1 << 3);
        writel(value, pgpio_dat_addr);
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
    int value = 0;
    struct device_node *plednode = NULL;

    ret = misc_register(&led_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    //在设备中查找对应节点
    plednode = of_find_node_by_path("/myled");
    if (NULL == plednode) {
        pr_info("of_find_node_by_path failed\n");
        return -1;
    }

    pmux_ctl_addr = of_iomap(plednode, 0);
    pmux_pad_addr = of_iomap(plednode, 1);
    pgpio_dir_addr = of_iomap(plednode, 2);
    pgpio_dat_addr = of_iomap(plednode, 3);

    value = readl(pmux_ctl_addr);
    value &= ~0xF;
    value |= 0x5;
    writel(value, pmux_ctl_addr);

    writel(0x10b0, pmux_pad_addr);

    value = readl(pgpio_dir_addr);
    value |= (0x1 << 3);
    writel(value, pgpio_dir_addr);

    value = readl(pgpio_dat_addr);
    value |= (0x1 << 3);
    writel(value, pgpio_dat_addr);

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