//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/atomic.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_25 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 25)
#define MIO_PIN_24 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 24)
#define MIO_PIN_12 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 12)
#define MIO_PIN_7 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 7)

//设置一个设备全局变量
struct lock_device
{
	dev_t devno;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	atomic64_t lock;
} lock_dev;

int lock_open(struct inode *inode, struct file *filp)
{
	printk("-lock_open-\n");
	if (!atomic64_read(&lock_dev.lock)) //读取锁的状态
		atomic64_inc(&lock_dev.lock);	//把原子变量加 1, 上锁
	else
		return -EBUSY; //若检测到已上锁，则返回设备忙
	return 0;
}
ssize_t lock_write(struct file *flip, const char __user *buf, size_t count, loff_t *fops)
{
	int flag = 0, i = 0;
	flag = copy_from_user(&i, buf, count); //使用copy_from_user读取用户态发送过来的数据
	printk(KERN_CRIT "flag = %d, i = %d, count = %ld\n", flag, i, count);
	if (flag != 0)
	{
		printk("Kernel receive data failed!\n");
		return 1;
	}
	if (i == 48)
	{
		gpio_set_value(MIO_PIN_25, 0);
		gpio_set_value(MIO_PIN_24, 0);
		gpio_set_value(MIO_PIN_12, 0);
		gpio_set_value(MIO_PIN_7, 0);
	}
	else
	{
		gpio_set_value(MIO_PIN_25, 1);
		gpio_set_value(MIO_PIN_24, 1);
		gpio_set_value(MIO_PIN_12, 1);
		gpio_set_value(MIO_PIN_7, 1);
	}
	return 0;
}
int lock_close(struct inode *inode, struct file *filp)
{
	printk("-lock_close-\n");
	atomic64_set(&lock_dev.lock, 0); //将变量设为0，意为解锁
	return 0;
}

const struct file_operations lock_fops = {
	.open = lock_open,
	.write = lock_write,
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

	//MIO_PIN_25 24 12 7申请GPIO口
	ret = gpio_request(MIO_PIN_25, "led1");
	if (ret < 0)
	{
		printk("gpio request led1 error!\n");
		return ret;
	}
	ret = gpio_request(MIO_PIN_24, "led2");
	if (ret < 0)
	{
		printk("gpio request led2 error!\n");
		return ret;
	}
	ret = gpio_request(MIO_PIN_12, "led3");
	if (ret < 0)
	{
		printk("gpio request led3 error!\n");
		return ret;
	}
	ret = gpio_request(MIO_PIN_7, "led4");
	if (ret < 0)
	{
		printk("gpio request led4 error!\n");
		return ret;
	}

	//GPIO口方向设置成输出
	ret = gpio_direction_output(MIO_PIN_25, 1);
	if (ret != 0)
	{
		printk("gpio direction output MIO_PIN_25 fail!\n");
	}
	ret = gpio_direction_output(MIO_PIN_24, 1);
	if (ret != 0)
	{
		printk("gpio direction output MIO_PIN_24 fail!\n");
	}
	ret = gpio_direction_output(MIO_PIN_12, 1);
	if (ret != 0)
	{
		printk("gpio direction output MIO_PIN_12 fail!\n");
	}
	ret = gpio_direction_output(MIO_PIN_7, 1);
	if (ret != 0)
	{
		printk("gpio direction output MIO_PIN_7 fail!\n");
	}

	//将原子变量置0，相当于初始化
	atomic64_set(&lock_dev.lock, 0);

	return 0;
}

static __exit void lock_drv_exit(void)
{
	printk("----^v^-----lock drv v1 exit\n");

	//释放按键GPIO
	gpio_free(MIO_PIN_25);
	gpio_free(MIO_PIN_24);
	gpio_free(MIO_PIN_12);
	gpio_free(MIO_PIN_7);

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