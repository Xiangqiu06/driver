#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define IOMUXC_CTL_ADDR     0x0229000C
#define IOMUXC_PAD_ADDR     0x02290050
#define GPIO_DIR_ADDR       0x020AC004
#define GPIO_DAT_ADDR       0x020AC000

static void __iomem *pmux_ctl_addr = NULL;
static void __iomem *pmux_pad_addr = NULL;
static void __iomem *pgpio_dir_addr = NULL;
static void __iomem *pgpio_dat_addr = NULL;

static ssize_t beep_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    int value = 0;
    long nret = 0;
    int beepstat = 0;

    nret = copy_from_user(&beepstat, puser, n);
    if (nret) {
        pr_info("copy_from_user failed\n");
        return -1;
    }

    if (1 == beepstat) {
        value = readl(pgpio_dat_addr);
        value &= ~(0x1 << 1);
        writel(value, pgpio_dat_addr);
    } else if (0 == beepstat) {
        value = readl(pgpio_dat_addr);
        value |= (0x1 << 1);
        writel(value, pgpio_dat_addr);
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
    int value = 0;

    ret = misc_register(&beep_misc);
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }

    pmux_ctl_addr = ioremap(IOMUXC_CTL_ADDR, 4);
    if (NULL == pmux_ctl_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pmux_pad_addr = ioremap(IOMUXC_PAD_ADDR, 4);
    if (NULL == pmux_pad_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pgpio_dir_addr = ioremap(GPIO_DIR_ADDR, 4);
    if (NULL == pgpio_dir_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pgpio_dat_addr = ioremap(GPIO_DAT_ADDR, 4);
    if (NULL == pgpio_dat_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    value = readl(pmux_ctl_addr);
    value &= ~0xF;
    value |= 0x5;
    writel(value, pmux_ctl_addr);

    writel(0x10b0, pmux_pad_addr);

    value = readl(pgpio_dir_addr);
    value |= (0x1 << 1);
    writel(value, pgpio_dir_addr);

    value = readl(pgpio_dat_addr);
    value |= (0x1 << 1);
    writel(value, pgpio_dat_addr);

    return 0;
}

static void __exit beep_misc_exit(void)
{
    misc_deregister(&beep_misc);
}

module_init(beep_misc_init);
module_exit(beep_misc_exit);

MODULE_LICENSE("GPL");
