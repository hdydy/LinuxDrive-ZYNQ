//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

static struct class *rtc_cls;
#define RTC_MAJOR 300
#define RTC_MINOR 0

//设计一个全局数据对象
struct rtc_ds1337_device
{
	struct cdev cdev;
	struct i2c_client *client; //记录匹配成功后的i2c client
};

struct rtc_ds1337_device *rtc_dev;

int rtc_i2c_send(struct i2c_client *client, char *buf, int count)
{
	int ret = 0;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr; //消息包是发送给哪个从设备
	msg.buf = buf;			 //传送的数据
	msg.flags = 0;			 //是发送数据还是接收数据
	msg.len = count;		 //数据的个数

	//参数1---i2c控制器对象---来自于client
	//参数2---统一的数据包
	//参数3---数据包的个数
	//返回值已经正确传输数据的个数
	ret = i2c_transfer(adapter, &msg, 1);
	return 0;
}

int rtc_i2c_recv(struct i2c_client *client, char *buf, int count)
{
	int ret = 0;
	struct i2c_adapter *adapter = client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr; //消息包是发送给哪个从设备
	msg.buf = buf;			 //传送的数据
	msg.flags = 1;			 //是发送数据还是接收数据
	msg.len = count;		 //数据的个数

	//参数1---i2c控制器对象---来自于client
	//参数2---统一的数据包
	//参数3---数据包的个数
	//返回值已经正确传输数据的个数
	ret = i2c_transfer(adapter, &msg, 1);
	return 0;
}

ssize_t rtc_ds1337_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int ret = 0;
	char *temp_buf = kzalloc(count, GFP_KERNEL);

	//先从从设备读取数据
	ret = rtc_i2c_recv(rtc_dev->client, temp_buf, count);
	if (ret < 0)
	{
		printk("rtc i2c recv error!\n");
		return -1;
	}

	//将数据传输到应用层
	ret = copy_to_user(buf, temp_buf, count);
	if (ret > 0)
	{
		printk("copy to user error!\n");
		return -EFAULT;
	}

	kfree(temp_buf);

	return count;
}

ssize_t rtc_ds1337_write(struct file *filp, const char __user *buf, size_t count, loff_t *fops)
{
	int ret = 0;
	char *temp_buf = kzalloc(count, GFP_KERNEL);

	//先从应用层获取数据
	ret = copy_from_user(temp_buf, buf, count);
	if (ret > 0)
	{
		printk("copy from user error!\n");
		return -EFAULT;
	}

	//将数据发送到底层
	ret = rtc_i2c_send(rtc_dev->client, temp_buf, count);
	if (ret < 0)
	{
		printk("rtc i2c send error!\n");
		return -1;
	}

	kfree(temp_buf);

	return count;
}

const struct file_operations rtc_fops = {
	.owner = THIS_MODULE,
	.read = rtc_ds1337_read,
	.write = rtc_ds1337_write,
};

//8、驱动匹配到硬件后自动进入probe函数
int rtc_ds1337_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	printk("rtc ds1337 probe!\n");

	//通过手动分配，组合主次设备号
	dev_t devno = MKDEV(RTC_MAJOR, RTC_MINOR);

	//给全局数据分配空间
	rtc_dev = kzalloc(sizeof(*rtc_dev), GFP_KERNEL);
	if (rtc_dev == NULL)
	{
		printk("error,Failed to allocate eeprom_dev space!\n");
		return -1;
	}

	//记录i2c匹配成功的client
	rtc_dev->client = client;

	//注册字符设备
	//参数一：注册的主设备
	//参数二：注册的设备共有多少次设备号
	//参数三：设备名字，在/proc/devices显示
	ret = register_chrdev_region(devno, 1, "rtc_ds1337");
	if (ret < 0)
	{
		printk("register chrdev region error!\n");
		return 0;
	}

	//初始化字符设备
	cdev_init(&rtc_dev->cdev, &rtc_fops);

	rtc_dev->cdev.owner = THIS_MODULE;

	//创建设备
	rtc_cls = class_create(rtc_dev->cdev.owner, "rtc_class");
	device_create(rtc_cls, NULL, devno, NULL, "rtc_device%d", 0);

	//向系统注册一个设备
	cdev_add(&rtc_dev->cdev, devno, 1);
	if (ret < 0)
	{
		printk("cdev add faile!\n");
		return -1;
	}

	return 0;
}

int rtc_ds1337_remove(struct i2c_client *clinet)
{
	dev_t devno = MKDEV(RTC_MAJOR, RTC_MINOR);
	cdev_del(&rtc_dev->cdev);
	//删除设备
	device_destroy(rtc_cls, devno);
	//删除类
	class_destroy(rtc_cls);
	unregister_chrdev_region(devno, 1);
	kfree(rtc_dev);
	return 0;
}

//7、用于驱动从设备树中匹配相应的硬件
const struct of_device_id of_rtc_ds1337[] = {
	{.compatible = "dallas,ds1337"},
};

//6、构建i2c driver
struct i2c_driver rtc_ds1337_drv = {

	.driver = {
		.owner = THIS_MODULE,
		.name = "rtc_ds1337",						  //体现在/sys/bus/i2c/driver/rtc_ds1337_drv
		.of_match_table = of_match_ptr(of_rtc_ds1337) //从设备树中根据compatible获取相应的硬件信息
	},
	.probe = rtc_ds1337_probe,
	.remove = rtc_ds1337_remove,
};

//4、实现装载入口函数
static __init int rtc_ds1337_drv_init(void)
{
	//构建i2c设备，并注册到i2c总线上
	return i2c_add_driver(&rtc_ds1337_drv);
}

//5、实现卸载入口函数
static __exit void rtc_ds1337_drv_exit(void)
{
	i2c_del_driver(&rtc_ds1337_drv);
}

//2、声明装载入口函数和卸载入口函数
module_init(rtc_ds1337_drv_init);
module_exit(rtc_ds1337_drv_exit);

//3、添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");