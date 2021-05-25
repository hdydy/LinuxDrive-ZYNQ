//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>

#define ZYNQMP_GPIO_NR_GPIOS 8
#define PS_LED_1 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 0)
#define PS_LED_2 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 1)
#define PS_LED_3 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 2)
#define PS_LED_4 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 3)

static int led_major;
static struct class *led_cls;
const struct file_operations led_fops = {};

//实现装载入口函数和卸载入口函数
static __init int led_drv_init(void)
{
	int ret = 0;
	printk("-led_drv_init start.-\n");

	//申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	led_major = register_chrdev(0, "led_drv", &led_fops);
	if (led_major < 0)
	{
		printk("Register chrdev faile!\n");
		return led_major;
	}
	printk("Register chrdev ok!\n");

	//自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	led_cls = class_create(THIS_MODULE, "led_class");
	printk("Class create ok!\n");

	//创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	device_create(led_cls, NULL, MKDEV(led_major, 0), NULL, "led_drv_%d", 0);
	printk("Device create ok!\n");

	//PS_LED申请GPIO口
	ret = gpio_request(PS_LED_1, "led1");
	if (ret < 0)
	{
		printk("gpio request led1 error!\n");
		return ret;
	}
	ret = gpio_request(PS_LED_2, "led2");
	if (ret < 0)
	{
		printk("gpio request led2 error!\n");
		return ret;
	}
	ret = gpio_request(PS_LED_3, "led3");
	if (ret < 0)
	{
		printk("gpio request led3 error!\n");
		return ret;
	}
	ret = gpio_request(PS_LED_4, "led4");
	if (ret < 0)
	{
		printk("gpio request led4 error!\n");
		return ret;
	}

	//全部灭灯
	gpio_direction_output(PS_LED_1, 0);
	gpio_direction_output(PS_LED_2, 0);
	gpio_direction_output(PS_LED_3, 0);
	gpio_direction_output(PS_LED_4, 0);

	printk("-led_drv_init finish.-\n");
	return 0;
}

static __exit void led_drv_exit(void)
{
	printk("-led_drv_exit start.-\n");

	//全部亮灯
	gpio_direction_output(PS_LED_1, 1);
	gpio_direction_output(PS_LED_2, 1);
	gpio_direction_output(PS_LED_3, 1);
	gpio_direction_output(PS_LED_4, 1);

	//释放GPIO口
	gpio_free(PS_LED_1);
	gpio_free(PS_LED_2);
	gpio_free(PS_LED_3);
	gpio_free(PS_LED_4);

	//删除设备
	device_destroy(led_cls, MKDEV(led_major, 0));
	//删除类
	class_destroy(led_cls);
	//注销主设备号
	unregister_chrdev(led_major, "led_drv");

	printk("-led_drv_exit finish.-\n");
}

//申明装载入口函数和卸载入口函数
module_init(led_drv_init);
module_exit(led_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");