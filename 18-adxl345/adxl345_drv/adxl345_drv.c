#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/spi/spi.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

/*
    adxl345首字节格式是bit7读(1)写(0)位 
    bit6 多字节连续读写（地址自动递增），0 为单字节
*/


struct spi_device *padxl345_spi = NULL;

static int adxl345_init(void)
{
    int ret = 0;
    unsigned char tx[2] = {0};
    unsigned char rx = {0};

    /*1. 读取设备ID */
    tx[0] = 0x00; //设备id寄存器
    tx[0] |= (1<<7);
    ret = spi_write_then_read(padxl345_spi,tx,1,&rx,1);
    if(ret){
        pr_info("spi_write_then_read failed\n");
        return -1;
    }
    /*2. 设置全分辨率 + 全量程 */
    tx[0] = 0x31;
    tx[1] = 0xB;
    ret = spi_write_then_read(padxl345_spi,tx,1,NULL,0);
    /*3. 开启采集 */
    tx[0] = 0x2D; //电源控制寄存器
    tx[1] = 0x08;
    ret = spi_write_then_read(padxl345_spi, tx, 2, NULL, 0);
    if(ret){
        pr_info("spi_write_then_read failed\n");
        return -1;
    }
    return 0;

}


static int adxl345_read_xyz(short *pxa, short *pya, short *pza)
{
    int ret = 0;
    unsigned char tx[7] = {0};//发送一个字节的控制读写命令，6个字节是xyz的数据
    unsigned char rx[7] = {0};
    struct spi_message sendmsg;
    struct spi_transfer transfer = {
        .tx_buf = tx,
        .rx_buf = rx,
        .len = 7,
        .delay_usecs = 20,
    };
    spi_message_init(&sendmsg);//初始化链表
    tx[0] = (0x32 | (0x1 << 7) | (0x1 << 6));//32地址下读，连续读
    spi_message_add_tail(&transfer, &sendmsg);//加入到链表中
    ret = spi_sync(padxl345_spi, &sendmsg);//执行顺序执行链表中的任务
    if (ret) {
        pr_info("spi_sync failed\n");
        return -1;
    }
    *pxa = (rx[2] << 8 | rx[1]);
    *pya = (rx[4] << 8 | rx[3]);
    *pza = (rx[6] << 8 | rx[5]);

    return 0;

}
static ssize_t adxl345_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    int ret = 0;
    short acc[3] = {0};
    int nret = 0;
    //对adxl345初始化
    ret = adxl345_init();
    if (-1 == ret) {
        pr_info("adxl345_init failed\n");
        return -1;
    }
    //2.读取ADXL345(X、Y、Z轴加速度)
    ret = adxl345_read_xyz(&acc[0], &acc[1], &acc[2]);
    //3.copy_to_user
    nret = copy_to_user(puser, acc, sizeof(acc));
    if (nret) {
        pr_info("copy_to_user failed\n");
        return -1;
    }

    return sizeof(acc);

}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read,
};

static struct miscdevice adxl345_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "adxl345_misc",
    .fops = &fops,
};

static int adxl345_probe(struct spi_device *spi)
{
    int ret = 0;
    pr_info("max_speed_hz = %d\n", spi->max_speed_hz);
    pr_info("chip_select = %d\n", spi->chip_select);   
    pr_info("bits_per_word = %d\n", spi->bits_per_word); 
    pr_info("mode = %d\n", spi->mode); 
    ret = misc_register(&adxl345_misc);
    if(ret)
    {
        pr_info("misc_register failed\n");
        return -1;
    }
    //出错的原因
    //padxl345_spi = spi;//内核根据设备树节点创建出来的"设备对象"，拿到设备的信息
    pr_info("probe ok!\n");
    return 0;
}

static int adxl345_remove(struct spi_device *spi)
{
    int ret = 0;

    ret = misc_deregister(&adxl345_misc);
    if (ret) {
        pr_info("misc_deregister failed\n");
        return -1;
    }   

    pr_info("remove ok!\n");

    return 0;
}

static struct spi_device_id adxl345_id_table[] = {
    {.name = "puteadxl345"},
    {},
};

static struct of_device_id adxl345_of_match_table[] = {
    {.compatible = "pute,puteadxl345"},
    {},
};

static struct spi_driver adxl345_drv = {
    .probe = adxl345_probe,
    .remove = adxl345_remove,
    .driver = {
        .name = "puteadxl345",
        .owner = THIS_MODULE,
        .of_match_table = adxl345_of_match_table,
    },
    .id_table = adxl345_id_table,
};




static int __init adxl345_drv_init(void)
{
    int ret = 0;
    ret = spi_register_driver(&adxl345_drv);
    if(ret){
        pr_info("spi_register_driver failed\n");
    }
    return 0;
}

static void __exit adxl345_drv_exit(void)
{
    spi_unregister_driver(&adxl345_drv);
    
    return ;
}

module_init(adxl345_drv_init);
module_exit(adxl345_drv_exit);

MODULE_LICENSE("GPL");