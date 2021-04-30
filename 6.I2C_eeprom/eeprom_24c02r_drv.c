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

#define EEPROM_MAJOR 400
#define EEPROM_MINOR 0

static struct class *EEPROM_cls;

//设计一个全局数据对象
struct eeprom_24c02r_device
{
	struct cdev cdev;
	struct i2c_client *client; //记录匹配成功后的i2c client
};

struct eeprom_24c02r_device *eeprom_dev;

int eeprom_i2c_send(struct i2c_client *client, char *buf, int count)
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

int eeprom_i2c_recv(struct i2c_client *client, char *buf, int count)
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

ssize_t eeprom_24c02r_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int ret = 0;
	char *temp_buf = kzalloc(count, GFP_KERNEL);

	//先从从设备读取数据
	ret = eeprom_i2c_recv(eeprom_dev->client, temp_buf, count);
	printk("Successful packet acquisition ret = %d\n", ret);
	if (ret < 0)
	{
		printk("eeprom i2c recv error!\n");
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

ssize_t eeprom_24c02r_write(struct file *filp, const char __user *buf, size_t count, loff_t *fops)
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
	ret = eeprom_i2c_send(eeprom_dev->client, temp_buf, count);
	printk("Successful packet acquisition ret = %d\n", ret);
	if (ret < 0)
	{
		printk("eeprom i2c send error!\n");
		return -1;
	}

	kfree(temp_buf);

	return count;
}

const struct file_operations eeprom_fops = {
	.owner = THIS_MODULE,
	.read = eeprom_24c02r_read,
	.write = eeprom_24c02r_write,
};

//8、驱动匹配到硬件后自动进入probe函数
int eeprom_24c02r_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	//通过手动分配，组合主次设备号
	dev_t devno = MKDEV(EEPROM_MAJOR, EEPROM_MINOR);

	//给全局数据分配空间
	eeprom_dev = kzalloc(sizeof(*eeprom_dev), GFP_KERNEL);
	if (eeprom_dev == NULL)
	{
		printk("error,Failed to allocate eeprom_dev space!\n");
		return -1;
	}

	//记录i2c匹配成功的client
	eeprom_dev->client = client;

	//注册字符设备
	//参数一：注册的主设备
	//参数二：注册的设备共有多少次设备号
	//参数三：设备名字，在/proc/devices显示
	ret = register_chrdev_region(devno, 1, "eeprom_24c02r");
	if (ret < 0)
	{
		printk("register chrdev region error!\n");
		return 0;
	}

	//初始化字符设备
	cdev_init(&eeprom_dev->cdev, &eeprom_fops);
	eeprom_dev->cdev.owner = THIS_MODULE;
	//创建类
	EEPROM_cls = class_create(THIS_MODULE, "EEPROM_class");
	//创建设备
	device_create(EEPROM_cls, NULL, devno, NULL, "EEPROM_device%d", 0);

	//向系统注册一个设备
	cdev_add(&eeprom_dev->cdev, devno, 1);
	if (ret < 0)
	{
		printk("cdev add faile!\n");
		return -1;
	}
	printk("Test Success!\n");
	return 0;
}

int eeprom_24c02r_remove(struct i2c_client *clinet)
{
	dev_t devno = MKDEV(EEPROM_MAJOR, EEPROM_MINOR);
	cdev_del(&eeprom_dev->cdev);
	//删除设备
	device_destroy(EEPROM_cls, devno);
	//删除类
	class_destroy(EEPROM_cls);
	unregister_chrdev_region(devno, 1);
	kfree(eeprom_dev);
	return 0;
}

//7、用于驱动从设备树中匹配相应的硬件
const struct of_device_id of_eeprom_24c02r[] = {
	{.compatible = "atmel,24c02"},
};

//6、构建i2c driver
struct i2c_driver eeprom_24c02r_drv = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "eeprom_24c02r",						 //体现在/sys/bus/i2c/driver/eeprom_24c02r_drv
		.of_match_table = of_match_ptr(of_eeprom_24c02r) //从设备树中根据compatible获取相应的硬件信息
	},
	.probe = eeprom_24c02r_probe,
	.remove = eeprom_24c02r_remove,
};

//4、实现装载入口函数
static __init int eeprom_24c02r_drv_init(void)
{
	//构建i2c设备，并注册到i2c总线上
	return i2c_add_driver(&eeprom_24c02r_drv);
}

//5、实现卸载入口函数
static __exit void eeprom_24c02r_drv_exit(void)
{
	i2c_del_driver(&eeprom_24c02r_drv);
}

//2、声明装载入口函数和卸载入口函数
module_init(eeprom_24c02r_drv_init);
module_exit(eeprom_24c02r_drv_exit);
//3、添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");