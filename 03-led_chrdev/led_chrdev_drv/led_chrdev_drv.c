#include <linux/fs.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <asm-generic/uaccess.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/slab.h>

typedef struct led_drv {
    void __iomem *pmux_ctl_addr;
    void __iomem *pmux_pad_addr;
    void __iomem *pgpio_dir_addr;
    void __iomem *pgpio_dat_addr;
    struct class *pclass;
    struct device *pdevice;
    int curledstat;
    struct mutex lock;
    dev_t devno;
    struct cdev *pcdev;
}led_drv_t;

extern int strcmp(const char *cs, const char *ct);
extern size_t strlen(const char *s);

#define IOMUXC_CTL_ADDR     0x020E0068
#define IOMUXC_PAD_ADDR     0x020E02F4
#define GPIO_DIR_ADDR       0x0209C004
#define GPIO_DAT_ADDR       0x0209C000

static led_drv_t *pled = NULL;

static ssize_t led_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int cnt = 0;

    mutex_lock(&pled->lock);
    cnt = sprintf(buf, "%s\n", (1 == pled->curledstat) ? "LED_ON" : "LED_OFF");
    mutex_unlock(&pled->lock);

    return cnt;
}

static ssize_t led_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    char tmpbuff[32] = {0};
    int value = 0;

    sscanf(buf, "%s", tmpbuff);

    mutex_lock(&pled->lock);
    
    if (0 == strcmp(tmpbuff, "LED_ON")) {
        value = readl(pled->pgpio_dat_addr);
        value &= ~(0x1 << 3);
        writel(value, pled->pgpio_dat_addr);

        pled->curledstat = 1;
    } else if (0 == strcmp(tmpbuff, "LED_OFF")) {
        value = readl(pled->pgpio_dat_addr);
        value |= (0x1 << 3);
        writel(value, pled->pgpio_dat_addr);

        pled->curledstat = 0;
    }

    mutex_unlock(&pled->lock);

    return count;
}

static struct device_attribute attr = __ATTR(attr, 0664, led_show, led_store);

static int led_open(struct inode *node, struct file *fp)
{
    pr_info("led_open success\n");

    return 0;
}

static int led_release(struct inode *node, struct file *fp)
{
    pr_info("led_release success\n");

    return 0;
}

static ssize_t led_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    unsigned long nret = 0;
    int value = 0;

    mutex_lock(&pled->lock);
    value = pled->curledstat;
    mutex_unlock(&pled->lock);
    
    nret = copy_to_user(puser, &value, sizeof(value));
    if (nret) {
        pr_info("copy_to_user failed\n");
        return -1;
    }

    pr_info("led_read success\n");

    return sizeof(pled->curledstat);
}

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

    mutex_lock(&pled->lock);
    if (1 == ledstat) {
        value = readl(pled->pgpio_dat_addr);
        value &= ~(0x1 << 3);
        writel(value, pled->pgpio_dat_addr);

        pled->curledstat = 1;
    } else if (0 == ledstat) {
        value = readl(pled->pgpio_dat_addr);
        value |= (0x1 << 3);
        writel(value, pled->pgpio_dat_addr);

        pled->curledstat = 0;
    }
    mutex_unlock(&pled->lock);

    pr_info("led_write success\n");

    return n;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .release = led_release,
    .read = led_read,
    .write = led_write,
};

static int __init led_chrdev_init(void)
{
    int ret = 0;
    int value = 0;

    //0.申请用户定义的led类型空间
    pled = kmalloc(sizeof(led_drv_t), GFP_KERNEL);
    if (NULL == pled) {
        pr_info("kmalloc failed\n");
        return -1;
    }

    //1.申请设备号   
    ret = alloc_chrdev_region(&pled->devno, 0, 1, "myled");
    if (ret) {
        pr_info("alloc_chrdev_region failed\n");
        return -1;
    }

    //2.申请cdev结构
    pled->pcdev = cdev_alloc();
    if (NULL == pled->pcdev) {
        pr_info("cdev_alloc failed\n");
        return -1;
    }

    //3.将file_operations中的对myled设备操作的方法(open、read、write、release)填充到cdev中
    pled->pcdev->ops = &fops;

    //4.将cdev结构添加到现有的主设备列表中
    ret = cdev_add(pled->pcdev, pled->devno, 1);
    if (ret) {
        pr_info("cdev_add failed\n");
        return -1;
    }

    pr_info("major:%d, minor:%d\n", MAJOR(pled->devno), MINOR(pled->devno));

    //5.在sys文件系统中创建myled设备类型
    pled->pclass = class_create(THIS_MODULE, "myled");
    if (NULL == pled->pclass) {
        pr_info("class_create failed\n");
        return -1;
    }

    //6.创建设备
    pled->pdevice = device_create(pled->pclass, NULL, pled->devno, NULL, "myled0");
    if (NULL == pled->pdevice) {
        pr_info("device_create failed\n");
        return -1;
    }

    //7.设备寄存器赋值
    pled->pmux_ctl_addr = ioremap(IOMUXC_CTL_ADDR, 4);
    if (NULL == pled->pmux_ctl_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pled->pmux_pad_addr = ioremap(IOMUXC_PAD_ADDR, 4);
    if (NULL == pled->pmux_pad_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pled->pgpio_dir_addr = ioremap(GPIO_DIR_ADDR, 4);
    if (NULL == pled->pgpio_dir_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    pled->pgpio_dat_addr = ioremap(GPIO_DAT_ADDR, 4);
    if (NULL == pled->pgpio_dat_addr) {
        pr_info("ioremap failed\n");
        return -1;
    }

    //1.配置引脚为GPIO功能
    value = readl(pled->pmux_ctl_addr);
    value &= ~0xF;
    value |= 0x5;
    writel(value, pled->pmux_ctl_addr);

    //2.配置引脚的电气属性
    writel(0x10b0, pled->pmux_pad_addr);

    //3.配置GPIO方向
    value = readl(pled->pgpio_dir_addr);
    value |= (0x1 << 3);
    writel(value, pled->pgpio_dir_addr);

    //4.配置GPIO的值(配置为1，关闭LED灯)
    value = readl(pled->pgpio_dat_addr);
    value |= (0x1 << 3);
    writel(value, pled->pgpio_dat_addr);

    pled->curledstat = 0;

    mutex_init(&pled->lock);

    //8.创建sys文件系统中调试节点
    ret = device_create_file(pled->pdevice, &attr);
    if (ret) {
        pr_info("device_create_file failed\n");
        return -1;
    }

    pr_info("led_chrdev_init success!\n");

    return 0;
}

static void __exit led_chrdev_exit(void)
{
    iounmap(pled->pgpio_dat_addr);
    iounmap(pled->pgpio_dir_addr);
    iounmap(pled->pmux_pad_addr);
    iounmap(pled->pmux_ctl_addr);

    mutex_destroy(&pled->lock);
    device_remove_file(pled->pdevice, &attr);
    device_destroy(pled->pclass, pled->devno);
    class_destroy(pled->pclass);
    cdev_del(pled->pcdev);
    unregister_chrdev_region(pled->devno, 1);
    kfree(pled);

    pr_info("led_chrdev_exit success!\n");

    return;
}

//驱动程序的入口
module_init(led_chrdev_init);

//驱动程序的出口
module_exit(led_chrdev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("pute");