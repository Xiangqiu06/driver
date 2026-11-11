#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
static struct i2c_client *lm75_client = NULL;
ssize_t lm75_read(struct file *fp, char __user *puser, size_t n, loff_t *off)
{
    struct i2c_msg sendmsg;
    struct i2c_msg recvmsg;
    int ret = 0;
    unsigned char tmpbuff[2] = {0};
    unsigned long nret = 0;
    memset(&sendmsg,0,sizeof(sendmsg));
    memset(&sendmsg,0,sizeof(sendmsg));

    sendmsg.addr = lm75_client->addr;
    tmpbuff[0] = 0x00;//
    sendmsg.buf = tmpbuff;
    sendmsg.len = 1;
    ret = lm75_client->adapter->algo->master_xfer(lm75_client->adapter, &sendmsg, 1);//调用底层I2C通信函数，发送一次I2C通信，完成一帧数据发送
    if (ret != 1) {
        pr_info("master_xfer sendmsg failed ret = %d\n", ret);
        return -1;
    }//在判断时判断错误导致直接返回-1了，抓波形明明发送成功了，从机给了ACK，但是读取不到数据
    
    recvmsg.addr = lm75_client->addr;
    recvmsg.flags |= I2C_M_RD;
    recvmsg.buf = tmpbuff;
    recvmsg.len =  2;
    ret = lm75_client->adapter->algo->master_xfer(lm75_client->adapter, &recvmsg, 1);//1 一次发送多少个消息数组
    if (ret != 1) {
        pr_info("master_xfer recvmsg failed ret = %d\n", ret);
        return -1;
    }
    nret = copy_to_user(puser, tmpbuff, sizeof(tmpbuff));
    if (nret) {
        pr_info("copy_to_user failed\n");
        return -1;
    }
    return sizeof(tmpbuff);
}
static struct file_operations fops = {
    .read  = lm75_read,
};

static struct miscdevice lm75_misc = {
    .minor = MISC_DYNAMIC_MINOR,//动态分配此设备号
    .name  = "lm75_misc",//混杂设备的名字
    .fops  = &fops,//操作方法
};
static int lm75_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = 0;
    lm75_client = client;               // 保存设备，read 里要用
    ret = misc_register(&lm75_misc);   // 最后注册，/dev/lm75_misc 出现
    if (ret) {
        pr_info("misc_register failed\n");
        return -1;
    }
    return 0;
}
static int lm75_remove(struct i2c_client *client)
{
    int ret = 0;
    
    ret = misc_deregister(&lm75_misc);
    if (ret) {
        pr_info("misc_deregister failed\n");
        return -1;
    }
    return 0;
}
struct of_device_id lm75_of_match[] = {
    { .compatible = "pute,putelm75" },   // 与设备树一字不差
    {},
};
static const struct i2c_device_id lm75_id[] = {
    { "putelm75", 0 },
    {}
};

static struct i2c_driver lm75_drv = {
    .probe = lm75_probe,//保存 client → 初始化硬件（如果需要）→ 注册 misc
    .remove = lm75_remove,//注销混杂设备
    .driver = {
        .name = "lm75_drv",//注册驱动的名字
        .owner = THIS_MODULE, 
        .of_match_table = lm75_of_match //设备树匹配规则，匹配成功就将设备与驱动绑定在一起
    },
    .id_table = lm75_id,//匹配设备树中的名字，匹配成功就将设备与驱动绑定在一起

};


//注册驱动
static int __init lm75_drv_init(void)
{
    return i2c_add_driver(&lm75_drv);// 注册：insmod 时（init 里）
}

static void __exit lm75_drv_exit(void)
{
    i2c_del_driver(&lm75_drv);// 注销：rmmod 时（exit 里）
    return;
}

module_init(lm75_drv_init);
module_exit(lm75_drv_exit);

MODULE_LICENSE("GPL");