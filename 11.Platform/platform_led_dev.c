//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define MIO_RES_BASE 338
#define MIO_RES_SIZE 2

//底层硬件信息存储在resource中
//start---需要传递MIO启始引脚值
//end-----传递多少个MIO
//name----资源名称
//flags---标志位IORESOURCE_IO表示IO资源，IORESOURCE_MEM表示内存资源
struct resource led_res[] = {
	[0] = {
		.start = MIO_RES_BASE,
		.end = MIO_RES_BASE + MIO_RES_SIZE,
		.name = "zynqmp_res",
		.flags = IORESOURCE_IO,
	}};

static void platform_device_led_release(struct device *dev)
{
}

//当dev和drv根据name在内核链表中进行配对，成功之后drv获取dev中的resource资源
static struct platform_device led_pdev = {
	.name = "zynqmp_led",
	.id = -1,
	.num_resources = ARRAY_SIZE(led_res),
	.resource = led_res,
	.dev = {
		.release = platform_device_led_release,
	}};

//实现装载入口函数和卸载入口函数
static __init int platform_led_dev_init(void)
{
	//创建pdev并添加到总线中
	return platform_device_register(&led_pdev);
}

static __exit void platform_led_dev_exit(void)
{
	//注销设备
	platform_device_unregister(&led_pdev);
}

//声明装载入口函数
module_init(platform_led_dev_init);
module_exit(platform_led_dev_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");