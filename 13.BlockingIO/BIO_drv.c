//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#include <linux/kernel.h>
#include <linux/ide.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <asm/io.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_42 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 42)

//设置一个设备全局变量
struct lock_device
{
	dev_t devno;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	atomic64_t lock;
	unsigned int irq;
	wait_queue_head_t waitqueue;
} lock_dev;

static irqreturn_t key_handler(int irq, void *dev)
{
	char value;
	value = gpio_get_value(MIO_PIN_42);
	if (value == 1)
	{
		atomic_set(&lock_dev.lock, 1);
		wake_up_interruptible(&lock_dev.waitqueue);
	}
}

int lock_open(struct inode *inode, struct file *filp)
{
	printk("-lock_open-\n");
	return 0;
}
ssize_t lock_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int ret = wait_event_interruptible(lock_dev.waitqueue, atomic64_read(&lock_dev.lock));
	int key = atomic64_read(&lock_dev.lock);
	copy_to_user(buf, &key, sizeof(key));
	atomic_set(&lock_dev.lock, 0);
	return 0;
}
int lock_close(struct inode *inode, struct file *filp)
{
	printk("-lock_close-\n");
	return 0;
}

const struct file_operations lock_fops = {
	.open = lock_open,
	.read = lock_read,
	.release = lock_close,
};

//实现装载入口函数和卸载入口函数
static __init int lock_drv_init(void)
{
	int ret = 0;
	printk("----^v^-----lock drv v1 init\n");

	//动态申请设备号
	ret = alloc_chrdev_region(&lock_dev.devno, 0, 1, "lock_device");
	if (ret < 0)
	{
		printk("alloc_chrdev_region fail!\n");
		return 0;
	}

	//设备初始化
	cdev_init(&lock_dev.cdev, &lock_fops);
	lock_dev.cdev.owner = THIS_MODULE;

	//自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	lock_dev.class = class_create(THIS_MODULE, "lock_class");
	if (IS_ERR(lock_dev.class))
	{
		printk("class_create fail!\n");
		return 0;
	}

	//创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	lock_dev.device = device_create(lock_dev.class, NULL, lock_dev.devno, NULL, "lock_device");
	if (IS_ERR(lock_dev.device))
	{
		printk("device_create fail!\n");
		return 0;
	}

	//向系统注册一个字符设备
	cdev_add(&lock_dev.cdev, lock_dev.devno, 1);

	//MIO_PIN_42申请GPIO口
	ret = gpio_request(MIO_PIN_42, "led1");
	if (ret < 0)
	{
		printk("gpio request led1 error!\n");
		return ret;
	}

	//GPIO口方向设置成输入
	ret = gpio_direction_input(MIO_PIN_42);
	if (ret != 0)
	{
		printk("gpio direction output MIO_PIN_42 fail!\n");
	}

	//将原子变量置0，相当于初始化
	atomic64_set(&lock_dev.lock, 0);

	lock_dev.irq = gpio_to_irq(MIO_PIN_42);
	ret = request_irq(lock_dev.irq, key_handler, IRQF_TRIGGER_RISING, "key", NULL);
	if (ret < 0)
	{
		printk("request_irq error!\n");
		return ret;
	}

	init_waitqueue_head(&lock_dev.waitqueue);

	return 0;
}

static __exit void lock_drv_exit(void)
{
	printk("----^v^-----lock drv v1 exit\n");

	//释放按键GPIO
	gpio_free(MIO_PIN_42);

	//注销字符设备
	cdev_del(&lock_dev.cdev);
	//删除设备节点
	device_destroy(lock_dev.class, lock_dev.devno);
	//删除设备类
	class_destroy(lock_dev.class);
	//注销设备号
	unregister_chrdev_region(lock_dev.devno, 1);
}

//申明装载入口函数和卸载入口函数
module_init(lock_drv_init);
module_exit(lock_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");