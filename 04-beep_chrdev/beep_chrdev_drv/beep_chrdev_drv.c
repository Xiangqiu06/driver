#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <asm-generic/uaccess.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>

typedef struct beep_drv {
    void __iomem *pmux_ctl_addr;
    void __iomem *pmux_pad_addr;
    void __iomem *pgpio_dir_addr;
    void __iomem *pgpio_dat_addr;
    struct class *pclass;
    struct device *pdevice;
    int curbeepstat;
    struct mutex lock;
    dev_t devno;
    struct cdev *pcdev;
}beep_drv_t;

extern int strcmp(const char *cs, const char *ct);
extern size_t strlen(const char *s);

#define IOMUXC_CTL_ADDR     0x0229000C
#define IOMUXC_PAD_ADDR     0x02290050
#define GPIO_DIR_ADDR       0x020AC004
#define GPIO_DAT_ADDR       0x020AC000

static beep_drv_t *pbeep = NULL;

static ssize_t beep_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int cnt = 0;

    mutex_lock(&pbeep->lock);
    cnt = sprintf(buf, "%s\n", (1 == pbeep->curbeepstat) ? "BEEP_ON" : "BEEP_OFF");
    mutex_unlock(&pbeep->lock);

    return cnt;
}

static ssize_t beep_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char tmpbuff[32] = {0};
    int value = 0;

    sscanf(buf, "%s", tmpbuff);

    mutex_lock(&pbeep->lock);
    
    if (0 == strcmp(tmpbuff, "BEEP_ON")) {
        value = readl(pbeep->pgpio_dat_addr);
        value &= ~(0x1 << 1);
        writel(value, pbeep->pgpio_dat_addr);

        pbeep->curbeepstat = 1;
    } else if (0 == strcmp(tmpbuff, "BEEP_OFF")) {
        value = readl(pbeep->pgpio_dat_addr);
        value |= (0x1 << 1);
        writel(value, pbeep->pgpio_dat_addr);

        pbeep->curbeepstat = 0;
    }

    mutex_unlock(&pbeep->lock);

    return count;
}

static struct device_attribute attr = __ATTR(attr, 0664, beep_show, beep_store);

static int beep_open(struct inode *node, struct file *fp)
{
    pr_info("beep_open success\n");

    return 0;
}

static int beep_release(struct inode *node, struct file *fp)
{
    pr_info("beep_release success\n");

    return 0;
}

static ssize_t beep_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
    int value = 0;

    mutex_lock(&pbeep->lock);
    value = pbeep->curbeepstat;
    mutex_unlock(&pbeep->lock);
    
    nret = copy_to_user(puser, &value, sizeof(value));
    if (nret) {
        pr_info("copy_to_user faibeep\n");
        return -1;
    }

    pr_info("beep_read success\n");

    return sizeof(pbeep->curbeepstat);
}

static ssize_t beep_write(struct file *fp, const char __user *puser, size_t n, loff_t *off)
{
    int value = 0;
    long nret = 0;
    int beepstat = 0;

    nret = copy_from_user(&beepstat, puser, n);
    if (nret) {
        pr_info("copy_from_user faibeep\n");
        return -1;
    }

    mutex_lock(&pbeep->lock);
    if (1 == beepstat) {
        value = readl(pbeep->pgpio_dat_addr);
        value &= ~(0x1 << 1);
        writel(value, pbeep->pgpio_dat_addr);

        pbeep->curbeepstat = 1;
    } else if (0 == beepstat) {
        value = readl(pbeep->pgpio_dat_addr);
        value |= (0x1 << 1);
        writel(value, pbeep->pgpio_dat_addr);

        pbeep->curbeepstat = 0;
    }
    mutex_unlock(&pbeep->lock);

    pr_info("beep_write success\n");

    return n;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .release = beep_release,
    .read = beep_read,
    .write = beep_write,
};

static int __init beep_chrdev_init(void)
{
    int ret = 0;
    int value = 0;

    //0.申请用户定义的beep类型空间
    pbeep = kmalloc(sizeof(beep_drv_t), GFP_KERNEL);
    if (NULL == pbeep) {
        pr_info("kmalloc faibeep\n");
        return -1;
    }

    //1.申请设备号   
    ret = alloc_chrdev_region(&pbeep->devno, 0, 1, "mybeep");
    if (ret) {
        pr_info("alloc_chrdev_region faibeep\n");
        return -1;
    }

    //2.申请cdev结构
    pbeep->pcdev = cdev_alloc();
    if (NULL == pbeep->pcdev) {
        pr_info("cdev_alloc faibeep\n");
        return -1;
    }

    //3.将file_operations中的对mybeep设备操作的方法(open、read、write、release)填充到cdev中
    pbeep->pcdev->ops = &fops;

    //4.将cdev结构添加到现有的主设备列表中
    ret = cdev_add(pbeep->pcdev, pbeep->devno, 1);
    if (ret) {
        pr_info("cdev_add faibeep\n");
        return -1;
    }

    pr_info("major:%d, minor:%d\n", MAJOR(pbeep->devno), MINOR(pbeep->devno));

    //5.在sys文件系统中创建mybeep设备类型
    pbeep->pclass = class_create(THIS_MODULE, "mybeep");
    if (NULL == pbeep->pclass) {
        pr_info("class_create faibeep\n");
        return -1;
    }

    //6.创建设备
    pbeep->pdevice = device_create(pbeep->pclass, NULL, pbeep->devno, NULL, "mybeep0");
    if (NULL == pbeep->pdevice) {
        pr_info("device_create faibeep\n");
        return -1;
    }

    //7.设备寄存器赋值
    pbeep->pmux_ctl_addr = ioremap(IOMUXC_CTL_ADDR, 4);
    if (NULL == pbeep->pmux_ctl_addr) {
        pr_info("ioremap faibeep\n");
        return -1;
    }

    pbeep->pmux_pad_addr = ioremap(IOMUXC_PAD_ADDR, 4);
    if (NULL == pbeep->pmux_pad_addr) {
        pr_info("ioremap faibeep\n");
        return -1;
    }

    pbeep->pgpio_dir_addr = ioremap(GPIO_DIR_ADDR, 4);
    if (NULL == pbeep->pgpio_dir_addr) {
        pr_info("ioremap faibeep\n");
        return -1;
    }

    pbeep->pgpio_dat_addr = ioremap(GPIO_DAT_ADDR, 4);
    if (NULL == pbeep->pgpio_dat_addr) {
        pr_info("ioremap faibeep\n");
        return -1;
    }

    //1.配置引脚为GPIO功能
    value = readl(pbeep->pmux_ctl_addr);
    value &= ~0xF;
    value |= 0x5;
    writel(value, pbeep->pmux_ctl_addr);

    //2.配置引脚的电气属性
    writel(0x10b0, pbeep->pmux_pad_addr);

    //3.配置GPIO方向
    value = readl(pbeep->pgpio_dir_addr);
    value |= (0x1 << 1);
    writel(value, pbeep->pgpio_dir_addr);

    //4.配置GPIO的值(配置为1，关闭beep)
    value = readl(pbeep->pgpio_dat_addr);
    value |= (0x1 << 1);
    writel(value, pbeep->pgpio_dat_addr);

    pbeep->curbeepstat = 0;

    mutex_init(&pbeep->lock);

    //8.创建sys文件系统中调试节点
    ret = device_create_file(pbeep->pdevice, &attr);
    if (ret) {
        pr_info("device_create_file faibeep\n");
        return -1;
    }

    pr_info("beep_chrdev_init success!\n");

    return 0;
}

static void __exit beep_chrdev_exit(void)
{
    iounmap(pbeep->pgpio_dat_addr);
    iounmap(pbeep->pgpio_dir_addr);
    iounmap(pbeep->pmux_pad_addr);
    iounmap(pbeep->pmux_ctl_addr);

    mutex_destroy(&pbeep->lock);
    device_remove_file(pbeep->pdevice, &attr);
    device_destroy(pbeep->pclass, pbeep->devno);
    class_destroy(pbeep->pclass);
    cdev_del(pbeep->pcdev);
    unregister_chrdev_region(pbeep->devno, 1);
    kfree(pbeep);

    pr_info("beep_chrdev_exit success!\n");

    return;
}

//驱动程序的入口
module_init(beep_chrdev_init);

//驱动程序的出口
module_exit(beep_chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pute");