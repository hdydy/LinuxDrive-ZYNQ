//填加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

//在probe函数中打印获取数据包里面的名字及GPIO
int led_pdrv_probe(struct platform_device *pdev)
{
	printk(KERN_CRIT "led pdrv probe!\n");
	printk(KERN_CRIT "GPIO  =  %lld\n", pdev->resource->start);
	printk(KERN_CRIT "led res name  =  %s\n", pdev->resource->name);
	return 0;
}

int led_pdrv_remove(struct platform_device *pdev)
{
	printk("led pdrv remove!\n");
	return 0;
}

//当drv在dev中找到name之后，进行配对获取resource资源，进入probe函数
struct platform_driver led_drv = {
	.driver = {
		.name = "zynqmp_led",
	},
	.probe = led_pdrv_probe,
	.remove = led_pdrv_remove,
};

//实现装载入口函数和卸载入口函数
static __init int platform_led_drv_init(void)
{
	//创建pdrv，并且注册到总线中
	return platform_driver_register(&led_drv);
}

static __exit void platform_led_drv_exit(void)
{
	//注销设备
	platform_driver_unregister(&led_drv);
}

//声明装载入口函数和卸载入口函数
module_init(platform_led_drv_init);
module_exit(platform_led_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");