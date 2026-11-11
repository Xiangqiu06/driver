#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/of_gpio.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/spinlock.h>

static int ds18b20gpiono = 0;
static spinlock_t lock;

extern void msleep(unsigned int msecs);

int master_recv_respone(void)
{
    int timeout = 0;
    gpio_direction_input(ds18b20gpiono);
    while(timeout < 50 && gpio_get_value(ds18b20gpiono)){
        udelay(10);
        timeout++;
    }

    if(timeout >= 50)
    {
        return -1;
    }

    while(!gpio_get_value(ds18b20gpiono));

    gpio_direction_output(ds18b20gpiono,1);
    return 0;
}

void master_send_reset(void)
{
    gpio_set_value(ds18b20gpiono,0);
    udelay(480);
    gpio_set_value(ds18b20gpiono,1);
    return;

}

static void master_send_byte(unsigned char tmpbyte)
{
    int i = 0;
    unsigned char flag = 0x01;

    gpio_direction_output(ds18b20gpiono,1);

    
    //0x1010 1010
    for(i = 0;i < 8;i++,flag <<= 1){
        gpio_set_value(ds18b20gpiono,0);
        udelay(5);

        if(tmpbyte & flag){
           gpio_set_value(ds18b20gpiono,1); //出现错误点，加以分析，问什么，之前写的0
        }

        udelay(55);
        gpio_set_value(ds18b20gpiono,1);
        udelay(5);
    }
}

static unsigned char master_recv_byte(void)
{
    int i = 0;
    unsigned char tmpbyte = 0x00;

    for(i = 0;i<8;i++)
    {
        gpio_direction_output(ds18b20gpiono,1);
        gpio_set_value(ds18b20gpiono,0);
        udelay(3);
        gpio_set_value(ds18b20gpiono,1);
        gpio_direction_input(ds18b20gpiono);
        udelay(8);
        if(gpio_get_value(ds18b20gpiono))
            tmpbyte |= (0x1 << i);
        gpio_direction_output(ds18b20gpiono,1);
        udelay(50);
        
    }
    return tmpbyte;
}

static ssize_t ds18b20_read(struct file *fp,char __user *puser,size_t n, loff_t *off)
{
    int ret = 0;
    unsigned char TH = 0;
    unsigned char TL = 0;
    unsigned long nret = 0;
    short temp = 0;
    unsigned long flags = 0;

    //1.发送复位
    master_send_reset();
    spin_lock_irqsave(&lock, flags);
    ret = master_recv_respone();
    if(ret){
        pr_info("master_recv_respone failed\n");
        return -1;
    }
    //2.发送0xcc
    master_send_byte(0xCC);
    //3。发送0x44
    master_send_byte(0x44);
    spin_unlock_irqrestore(&lock, flags);
    //4.睡眠400ms
    msleep(500);
    //5.发送0xcc
    master_send_reset();
    ret = master_recv_respone();
    if(ret){
        pr_info("master_recv_respone failed\n");
        return -1;
    }
    spin_lock_irqsave(&lock, flags);
    master_send_byte(0xCC);
    //6.发送0xbe
    master_send_byte(0xBE);
    //7.读取2个字节
    TL = master_recv_byte();
    TH = master_recv_byte();
    spin_unlock_irqrestore(&lock, flags);
    //8.返回给应用层
    temp = (TH << 8 | TL);

    nret = copy_to_user(puser,&temp,sizeof(temp));
    if(nret)
    {
        pr_info("copy_to_user failed\n");
        return -1;
    }

    //返回值写错，导致成功读取，返回0，应用层出现诡异的fail to read ds18b20: Success
    //要上逻辑分析仪
    return  sizeof(temp);
}
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = ds18b20_read,
};

static struct miscdevice ds18b20_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "ds18b20_misc",
    .fops = &fops,
};

static int ds18b20_probe(struct platform_device *pdevice)
{
    int ret = 0;

    ret = misc_register(&ds18b20_misc);
    if(ret){
        pr_info("misc_register failed\n");
        return -1;
    }
    ds18b20gpiono = of_get_named_gpio(pdevice->dev.of_node,"gpio-ds18b20",0);
    if(ds18b20gpiono < 0){
        pr_info("of_get_named_gpio failed\n");
        return -1;
    }

    ret = devm_gpio_request(ds18b20_misc.this_device,ds18b20gpiono,"ds18b20-drv");
    if(ret){
        pr_info("devm_gpio_request failed\n");
        return -1;
    }

    gpio_direction_output(ds18b20gpiono,1);

    return 0;
}
static int ds18b20_remove(struct platform_device *pdevice)
{
    int ret = 0;

    ret = misc_deregister(&ds18b20_misc);
    if(ret){
        pr_info("misc_deregister failed\n");
        return -1;
    }
    return 0;
}

static struct platform_device_id ds18b20_id_tables[] = {
    {.name = "puteds18b20"},
    {},
};

static struct of_device_id ds18b20_of_match_tables[] = {
    {.compatible = "pute,puteds18b20"},
    {},
};

static struct platform_driver ds18b20_drv = {
    .driver = {
        .name = "puteds18b20",
        .owner = THIS_MODULE,
        .of_match_table = ds18b20_of_match_tables,
    },
    .probe = ds18b20_probe,
    .remove = ds18b20_remove,
    .id_table = ds18b20_id_tables,

};


module_platform_driver(ds18b20_drv);

MODULE_LICENSE("GPL");