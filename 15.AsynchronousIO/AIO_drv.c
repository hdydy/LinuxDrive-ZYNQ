//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_42 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 42)

//设置一个设备全局变量
struct aio_device
{
	dev_t devno;					   //设备号
	struct cdev cdev;				   //字符设备
	struct class *class;			   //设备类
	struct device *device;			   //设备
	atomic64_t state;				   //原子变量，此处用作按键状态
	unsigned int irq;				   //中断
	wait_queue_head_t waitqueue;	   //等待队列
	struct fasync_struct *async_queue; //异步信号
} aio_dev;							   //定义一个全局的设备结构体

//中断回调函数，此处会当按键检测到上升沿触发
static irqreturn_t key_handler(int irq, void *dev_0)
{
	char value;
	//读取按键状态，按下为1，松开为0
	value = gpio_get_value(MIO_PIN_42);
	if (value == 1) //按下的情况
	{
		//设置原子变量为1
		atomic64_set(&aio_dev.state, 1);
		//唤醒等待队列
		wake_up_interruptible(&aio_dev.waitqueue);
		if (aio_dev.async_queue)	//发送信号，用来唤醒进程
			kill_fasync(&aio_dev.async_queue, SIGIO, POLL_IN);
	}
	return 0;
}

static int aio_fasync(int fd, struct file *filp, int mode)
{
	//初始化fasync_struct结构体
	return fasync_helper(fd, filp, mode, &aio_dev.async_queue);
}

int aio_open(struct inode *inode, struct file *filp)
{
	printk("-aio_open-\n");
	return 0;
}
ssize_t aio_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int ret = 0, key = 0;
	ret = wait_event_interruptible(aio_dev.waitqueue, atomic64_read(&aio_dev.state));
	if (ret)
		return ret;
	//上面的队列被唤醒后，读取当前按键状态
	key = atomic64_read(&aio_dev.state);
	//发送到用户空间
	ret = copy_to_user(buf, &key, sizeof(key));
	if (ret)
		return ret;
	//将状态置0，等待下一次中断
	atomic_set(&aio_dev.state, 0);
	return 0;
}
int aio_close(struct inode *inode, struct file *filp)
{
	printk("-aio_close-\n");
	return aio_fasync(-1, filp, 0);
}

const struct file_operations aio_fops = {
	.open = aio_open,
	.read = aio_read,
	.release = aio_close,
	.fasync = aio_fasync,
};

//实现装载入口函数和卸载入口函数
static __init int aio_drv_init(void)
{
	int ret = 0;
	printk("-aio_drv_init start-\n");

	//动态申请设备号
	ret = alloc_chrdev_region(&aio_dev.devno, 0, 1, "aio_device");
	if (ret < 0)
	{
		printk("alloc_chrdev_region fail!\n");
		return 0;
	}

	//设备初始化
	cdev_init(&aio_dev.cdev, &aio_fops);
	aio_dev.cdev.owner = THIS_MODULE;

	//自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	aio_dev.class = class_create(THIS_MODULE, "aio_class");
	if (IS_ERR(aio_dev.class))
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
	aio_dev.device = device_create(aio_dev.class, NULL, aio_dev.devno, NULL, "aio_device");
	if (IS_ERR(aio_dev.device))
	{
		printk("device_create fail!\n");
		return 0;
	}

	//向系统注册一个字符设备
	cdev_add(&aio_dev.cdev, aio_dev.devno, 1);

	//MIO_PIN_42申请GPIO口
	ret = gpio_request(MIO_PIN_42, "key");
	if (ret < 0)
	{
		printk("gpio request key error!\n");
		return ret;
	}

	//GPIO口方向设置成输入
	ret = gpio_direction_input(MIO_PIN_42);
	if (ret != 0)
	{
		printk("gpio direction input MIO_PIN_42 fail!\n");
	}

	//将原子变量置0，相当于初始化
	atomic64_set(&aio_dev.state, 0);

	//申请中断号
	aio_dev.irq = gpio_to_irq(MIO_PIN_42);
	//设置中断方式
	ret = request_irq(aio_dev.irq, key_handler, IRQF_TRIGGER_RISING, "key", NULL);
	if (ret < 0)
	{
		printk("request_irq error!\n");
		return ret;
	}

	//初始化等待队列头
	init_waitqueue_head(&aio_dev.waitqueue);

	printk("-aio_drv_init finish-\n");
	return 0;
}

static __exit void aio_drv_exit(void)
{
	printk("-aio_drv_exit start-\n");

	//释放中断
	free_irq(aio_dev.irq, NULL);
	//释放按键GPIO
	gpio_free(MIO_PIN_42);
	//注销字符设备
	cdev_del(&aio_dev.cdev);
	//删除设备节点
	device_destroy(aio_dev.class, aio_dev.devno);
	//删除设备类
	class_destroy(aio_dev.class);
	//注销设备号
	unregister_chrdev_region(aio_dev.devno, 1);

	printk("-aio_drv_exit finish-\n");
}

//申明装载入口函数和卸载入口函数
module_init(aio_drv_init);
module_exit(aio_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");