//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_42 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 42)
#define MIO_PIN_40 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 40)
#define MIO_PIN_44 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 44)
#define MIO_PIN_36 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 36)

static int key_major;
static int ret = 0;
static struct class *key_cls;

int key_open(struct inode *inode, struct file *filp)
{
	printk("----^v^-----key open\n");
	//将MIN42 44 40 36设置为输入，
	ret = gpio_direction_input(MIO_PIN_42);
	if (ret != 0)
	{
		printk("gpio direction input MIO_PIN_42 fail!\n");
	}
	ret = gpio_direction_input(MIO_PIN_40);
	if (ret != 0)
	{
		printk("gpio direction input MIO_PIN_40 fail!\n");
	}
	ret = gpio_direction_input(MIO_PIN_44);
	if (ret != 0)
	{
		printk("gpio direction input MIO_PIN_44 fail!\n");
	}
	ret = gpio_direction_input(MIO_PIN_36);
	if (ret != 0)
	{
		printk("gpio direction input MIO_PIN_36 fail!\n");
	}

	return 0;
}

ssize_t key_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int value[4] = {0};
	printk("----^v^-----key read\n");
	value[0] = gpio_get_value(MIO_PIN_42);
	value[1] = gpio_get_value(MIO_PIN_40);
	value[2] = gpio_get_value(MIO_PIN_44);
	value[3] = gpio_get_value(MIO_PIN_36);

	ret = copy_to_user(buf, value, count);
	if (ret < 0)
	{
		printk("gpio get value failed!\n");
		return ret;
	}

	return 0;
}

int key_close(struct inode *inode, struct file *filp)
{
	printk("----^v^-----key close\n");
	return 0;
}

const struct file_operations key_fops = {
	.open = key_open,
	.read = key_read,
	.release = key_close,
};

//4、实现装载入口函数和卸载入口函数
static __init int key_drv_v1_init(void)
{
	printk("----^v^-----key drv v1 init\n");
	//①、申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	key_major = register_chrdev(0, "key_drv_v1", &key_fops);
	if (key_major < 0)
	{
		printk("register chrdev faile!\n");
		return key_major;
	}
	printk("register chrdev ok!\n");

	//②、自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	key_cls = class_create(THIS_MODULE, "key_class");
	printk("class create ok!\n");

	//③、创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	device_create(key_cls, NULL, MKDEV(key_major, 0), NULL, "mykey%d", 0);
	printk("device create ok!\n");

	//④、MIO_PIN_42 40 44 36申请GPIO口
	ret = gpio_request(MIO_PIN_42, "key1");
	if (ret < 0)
	{
		printk("gpio request key1 error!\n");
		return ret;
	}

	ret = gpio_request(MIO_PIN_40, "key2");
	if (ret < 0)
	{
		printk("gpio request key2 error!\n");
		return ret;
	}

	ret = gpio_request(MIO_PIN_44, "key3");
	if (ret < 0)
	{
		printk("gpio request key3 error!\n");
		return ret;
	}

	ret = gpio_request(MIO_PIN_36, "key4");
	if (ret < 0)
	{
		printk("gpio request key4 error!\n");
		return ret;
	}

	return 0;
}

static __exit void key_drv_v1_exit(void)
{
	printk("----^v^-----key drv v1 exit\n");

	//释放按键GPIO
	gpio_free(MIO_PIN_42);
	gpio_free(MIO_PIN_40);
	gpio_free(MIO_PIN_44);
	gpio_free(MIO_PIN_36);

	//删除设备
	device_destroy(key_cls, MKDEV(key_major, 0));
	//删除类
	class_destroy(key_cls);
	//注销主设备号
	unregister_chrdev(key_major, "key_drv_v1");
}

//2、申明装载入口函数和卸载入口函数
module_init(key_drv_v1_init);
module_exit(key_drv_v1_exit);

//3、添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");