//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

static unsigned int led_major;
static struct class *led_class;

struct led_driver
{
	int gpio1;
	int gpio2;
	int gpio3;
	int gpio4;
	int irq;
	struct device dev;
};
struct led_driver *led_dri = NULL;

const struct file_operations led_fops = {

};

//在probe函数中打印获取数据包里面的名字及GPIO
int gpio_pdrv_probe(struct platform_device *pdev)
{
	struct device_node *node;
	unsigned int gpio1, gpio2, gpio3, gpio4;
	unsigned int ret = 0;

	printk("gpio pdrv probe!\n");
	printk("pdrv name = %s!\n", pdev->name);

	//申请主设备号
	led_major = register_chrdev(0, "led_drv", &led_fops);
	if (led_major < 0)
	{
		printk("register chrdev led major error!\n");
		return -ENOMEM;
	}

	//创建类
	led_class = class_create(THIS_MODULE, "led_class");

	//创建设备
	device_create(led_class, NULL, MKDEV(led_major, 0), NULL, "led_device%d", 0);

	//硬件初始化

	//申请空间
	led_dri = devm_kmalloc(&pdev->dev, sizeof(struct led_driver), GFP_KERNEL);
	if (led_dri == NULL)
	{
		printk("devm kmalloc led_driver error!\n");
		return -1;
	}

	//获取从设备节点传过来的pdev中的dev及node节点
	led_dri->dev = pdev->dev;
	node = pdev->dev.of_node;

	//从node节点处获得GPIO号
	gpio1 = of_get_gpio(node, 0);
	printk("of get gpio1 number = %d\n", gpio1);
	if (gpio1 < 0)
	{
		printk("of get gpio error!\n");
		return -1;
	}
	gpio2 = of_get_gpio(node, 1);
	printk("of get gpio2 number = %d\n", gpio2);
	if (gpio2 < 0)
	{
		printk("of get gpio error!\n");
		return -1;
	}
	gpio3 = of_get_gpio(node, 2);
	printk("of get gpio3 number = %d\n", gpio3);
	if (gpio3 < 0)
	{
		printk("of get gpio error!\n");
		return -1;
	}
	gpio4 = of_get_gpio(node, 3);
	printk("of get gpio4 number = %d\n", gpio4);
	if (gpio4 < 0)
	{
		printk("of get gpio error!\n");
		return -1;
	}

	//申请GPIO
	ret = gpio_request(gpio1, "plattree_led");
	if (ret < 0)
	{
		printk("plattree led gpio request error!\n");
		return ret;
	}
	ret = gpio_request(gpio2, "plattree_led");
	if (ret < 0)
	{
		printk("plattree led gpio request error!\n");
		return ret;
	}
	ret = gpio_request(gpio3, "plattree_led");
	if (ret < 0)
	{
		printk("plattree led gpio request error!\n");
		return ret;
	}
	ret = gpio_request(gpio4, "plattree_led");
	if (ret < 0)
	{
		printk("plattree led gpio request error!\n");
		return ret;
	}

	//设置GPIO为输出模式，并设备为0，灭灯
	gpio_direction_output(gpio1, 0);
	gpio_direction_output(gpio2, 0);
	gpio_direction_output(gpio3, 0);
	gpio_direction_output(gpio4, 0);

	led_dri->gpio1 = gpio1;
	led_dri->gpio2 = gpio2;
	led_dri->gpio3 = gpio3;
	led_dri->gpio4 = gpio4;

	return 0;
}

int gpio_pdrv_remove(struct platform_device *pdev)
{
	printk("led pdrv remove!\n");

	gpio_set_value(led_dri->gpio1, 1);
	gpio_set_value(led_dri->gpio2, 1);
	gpio_set_value(led_dri->gpio3, 1);
	gpio_set_value(led_dri->gpio4, 1);

	gpio_free(led_dri->gpio1);
	gpio_free(led_dri->gpio2);
	gpio_free(led_dri->gpio3);
	gpio_free(led_dri->gpio4);
	device_destroy(led_class, MKDEV(led_major, 0));
	class_destroy(led_class);
	unregister_chrdev(led_major, "led_drv");

	return 0;
}

//of_match_table实现
const struct of_device_id gpio_of_match_table[] = {
	{
		.compatible = "xlnx,zynqmp-led-1.0",
	},
	{}};

//当驱动在设备中找到name之后，进行配对获取resource资源，进入probe函数
struct platform_driver gpio_drv = {
	.driver = {
		.name = "zynqmp_led",
		.of_match_table = gpio_of_match_table,
	},
	.probe = gpio_pdrv_probe,
	.remove = gpio_pdrv_remove,
};

//实现装载入口函数和卸载入口函数
static __init int platform_gpio_drv_init(void)
{
	//创建pdrv，并且注册到总线中
	return platform_driver_register(&gpio_drv);
}
static __exit void platform_gpio_drv_exit(void)
{
	//注销设备
	platform_driver_unregister(&gpio_drv);
}

//声明装载入口函数和卸载入口函数
module_init(platform_gpio_drv_init);
module_exit(platform_gpio_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");