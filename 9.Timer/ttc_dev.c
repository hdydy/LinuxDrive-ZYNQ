//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ide.h>
#include <linux/cdev.h>
#include <linux/timer.h>

//设置一个设备全局变量
struct timer_device
{
    dev_t devno;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    unsigned long count;
    struct timer_list timer;
};
struct timer_device timer_dev;

//timer回调函数，倒计时结束后会运行回调函数
static void timer_callback(struct timer_list *timer)
{
    printk(KERN_CRIT "timer_callback.\n");
}

//打开设备函数
static int timer_open(struct inode *inode, struct file *filp)
{
    printk("timer_open success.\n");
    return 0;
}
//接收用户空间传来的数据
static ssize_t timer_write(struct file *filp, const char __user *buf, size_t count, loff_t *fops)
{
    int flag = 0, i = 0; //flag用来判断是否读取成功，i为一个计数器
    unsigned long temp;  //用来临时存放计算值
    timer_dev.count = 0;
    flag = copy_from_user(&temp, buf, count); //使用copy_from_user读取用户态发送过来的数据
    if (flag != 0)
    {
        printk("Kernel receive data failed!\n");
        return 1;
    }
    //将char*转为unsigned long
    for (i = 0; i < count; i++)
    {
        timer_dev.count *= 10;
        timer_dev.count += temp % 256 - 48;
        temp = temp >> 8;
    }
    printk(KERN_CRIT "wait time = %ld\n", timer_dev.count);
    //设置定时器的延时
    mod_timer(&timer_dev.timer, jiffies + msecs_to_jiffies(timer_dev.count));
    return 0;
}
//关闭设备函数
static int timer_release(struct inode *inode, struct file *filp)
{
    printk("timer_release success.\n");
    return 0;
}

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .write = timer_write,
    .release = timer_release,
};

//实现装载入口函数
static __init int timer_drv_init(void)
{
    int ret = 0;
    printk("timer_drv_init start.\n");

    //动态申请设备号
    ret = alloc_chrdev_region(&timer_dev.devno, 0, 1, "timer_device");
    if (ret < 0)
    {
        printk("alloc_chrdev_region fail!\n");
        return 0;
    }

    //设备初始化
    cdev_init(&timer_dev.cdev, &timer_fops);
    timer_dev.cdev.owner = THIS_MODULE;

    //申请创建设备类
    timer_dev.class = class_create(THIS_MODULE, "timer_class");
    if (IS_ERR(timer_dev.class))
    {
        printk("class_create fail!\n");
        return 0;
    }

    //建立设备节点
    timer_dev.device = device_create(timer_dev.class, NULL, timer_dev.devno, NULL, "timer_device");
    if (IS_ERR(timer_dev.device))
    {
        printk("device_create fail!\n");
        return 0;
    }

    //向系统注册一个字符设备
    cdev_add(&timer_dev.cdev, timer_dev.devno, 1);

    //初始化定时器
    timer_setup(&timer_dev.timer, timer_callback, 0);

    printk("timer_drv_init success.\n");
    return 0;
}

//实现卸载入口函数
static __exit void timer_drv_exit(void)
{
    printk("timer_drv_exit start.\n");
    //删除定时器
    del_timer_sync(&timer_dev.timer);
    //注销字符设备
    cdev_del(&timer_dev.cdev);
    //删除设备节点
    device_destroy(timer_dev.class, timer_dev.devno);
    //删除设备类
    class_destroy(timer_dev.class);
    //注销设备号
    unregister_chrdev_region(timer_dev.devno, 1);
    printk("timer_drv_exit success.\n");
}

//声明装载入口函数和卸载入口函数
module_init(timer_drv_init);
module_exit(timer_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");