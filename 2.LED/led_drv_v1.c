//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>

static int led_major;
static struct class *led_cls;

/* gpio 内存映射地址 */
static unsigned long *gpio_addr = NULL;
static unsigned long *iou_slcr = NULL;

/* gpio 寄存器物理基地址 */
#define GPIO_BASE_ADDR 0xFF0A0000
#define IOU_SLCR_ADDR 0xFF180000

/* gpio 寄存器所占空间大小 */
#define GPIO_BASE_SIZE 0x368
#define IOU_SLCR_SIZE 0x714

/* gpio 引脚设置GPIO功能*/
#define GPIO_PIN_7 (unsigned int *)((unsigned long)iou_slcr + 0x0000001C)
#define GPIO_PIN_12 (unsigned int *)((unsigned long)iou_slcr + 0x00000030)
#define GPIO_PIN_24 (unsigned int *)((unsigned long)iou_slcr + 0x00000060)
#define GPIO_PIN_25 (unsigned int *)((unsigned long)iou_slcr + 0x00000064)

/* gpio 控制电流0*/
#define IOU_SLCR_BANK0_CTRL0 (unsigned int *)((unsigned long)iou_slcr + 0x00000138)
/* gpio 控制电流1*/
#define IOU_SLCR_BANK0_CTRL1 (unsigned int *)((unsigned long)iou_slcr + 0x0000013c)
/* gpio 输入引脚选择Schmitt或CMOS*/
#define IOU_SLCR_BANK0_CTRL3 (unsigned int *)((unsigned long)iou_slcr + 0x00000140)
/* gpio 设置上下拉*/
#define IOU_SLCR_BANK0_CTRL4 (unsigned int *)((unsigned long)iou_slcr + 0x00000144)
/* gpio 使能上下拉*/
#define IOU_SLCR_BANK0_CTRL5 (unsigned int *)((unsigned long)iou_slcr + 0x00000148)
/* gpio 引脚速率选择*/
#define IOU_SLCR_BANK0_CTRL6 (unsigned int *)((unsigned long)iou_slcr + 0x0000014c)

/* gpio 方向寄存器 */
#define GPIO_DIRM_0 (unsigned int *)((unsigned long)gpio_addr + 0x00000204)
/* gpio 使能寄存器 */
#define GPIO_OEN_0 (unsigned int *)((unsigned long)gpio_addr + 0x00000208)
/* gpio 输出数据寄存器 */
#define GPIO_DATA_0 (unsigned int *)((unsigned long)gpio_addr + 0x00000040)
/* gpio 输出数据控制寄存器1 */
#define GPIO_MASK_DATA_0_LSW (unsigned int *)((unsigned long)gpio_addr + 0x00000000)
/* gpio 输出数据控制寄存器2 */
#define GPIO_MASK_DATA_0_MSW (unsigned int *)((unsigned long)gpio_addr + 0x00000004)

const struct file_operations led_fops = {};

//实现装载入口函数和卸载入口函数
static __init int led_drv_v1_init(void)
{
	printk("-------^v^-------\n");
	printk("-led drv v1 init-\n");
	//申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	led_major = register_chrdev(0, "led_drv_v1", &led_fops);
	if (led_major < 0)
	{
		printk("register chrdev faile!\n");
		return led_major;
	}
	printk("register chrdev ok!\n");

	//动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	led_cls = class_create(THIS_MODULE, "led_class");
	printk("class create ok!\n");

	//创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	device_create(led_cls, NULL, MKDEV(led_major, 0), NULL, "Myled%d", 0);
	printk("device create ok!\n");

	//硬件初始化
	//内存映射
	gpio_addr = ioremap(GPIO_BASE_ADDR, GPIO_BASE_SIZE);
	iou_slcr = ioremap(IOU_SLCR_ADDR, IOU_SLCR_SIZE);
	printk("gpio_addr init over!\n");

	/* MIO7 MIO12 设置成GPIO */
	iowrite32((ioread32(GPIO_PIN_7) & 0x0), GPIO_PIN_7);
	printk("gpio MIO7 init over!\n");
	iowrite32((ioread32(GPIO_PIN_12) & 0x0), GPIO_PIN_12);
	printk("gpio MIO12 init over!\n");
	iowrite32((ioread32(GPIO_PIN_24) & 0x0), GPIO_PIN_24);
	printk("gpio MIO24 init over!\n");
	iowrite32((ioread32(GPIO_PIN_25) & 0x0), GPIO_PIN_25);
	printk("gpio MIO25 init over!\n");

	/* MIO7 MIO12设置输出驱动电流大小 */
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL0) | 0x3FFFFFF), IOU_SLCR_BANK0_CTRL0);
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL1) & 0x0), IOU_SLCR_BANK0_CTRL1);

	/* 选择引脚是Schmitt还是CMOS */
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL3) & 0x0), IOU_SLCR_BANK0_CTRL3);

	/* 输出管脚上下拉及使能 */
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL4) | 0x3FFFFFF), IOU_SLCR_BANK0_CTRL4);
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL5) | 0x3FFFFFF), IOU_SLCR_BANK0_CTRL5);

	/* MIO速率的选择 */
	iowrite32((ioread32(IOU_SLCR_BANK0_CTRL6) & 0x0), IOU_SLCR_BANK0_CTRL6);

	/* MIO7 MIO12 设置成输出 */
	iowrite32((ioread32(GPIO_DIRM_0) | 0x03001080), GPIO_DIRM_0);
	/*  MIO7 MIO12 使能 */
	iowrite32((ioread32(GPIO_OEN_0) | 0x03001080), GPIO_OEN_0);

	/* MASK_DATA方式按灭LED0，LED1 */
	//iowrite32((ioread32(GPIO_MASK_DATA_0_LSW ) & 0xFFFFEF7F), GPIO_MASK_DATA_0_LSW );
	//printk("GPIO_MASK_DATA_0_LSW = 0x%x\n", *GPIO_MASK_DATA_0_LSW);
	//iowrite32((ioread32(GPIO_MASK_DATA_0_MSW ) & 0xFFFFFCFF), GPIO_MASK_DATA_0_MSW );
	//printk("GPIO_MASK_DATA_0_MSW = 0x%x\n", *GPIO_MASK_DATA_0_MSW);
	/* DATA方式按灭LED0，LED1*/
	iowrite32((ioread32(GPIO_DATA_0) & 0xFCFFEF7F), GPIO_DATA_0);
	printk("GPIO_DATA_0 = 0x%x\n", *GPIO_DATA_0);

	return 0;
}

static __exit void led_drv_v1_exit(void)
{
	iowrite32((ioread32(GPIO_DATA_0) | 0xFFFFFFFF), GPIO_DATA_0);
	//内存释放
	iounmap(gpio_addr);
	iounmap(iou_slcr);
	//删除设备
	device_destroy(led_cls, MKDEV(led_major, 0));
	//删除类
	class_destroy(led_cls);
	//注销主设备号
	unregister_chrdev(led_major, "led_drv_v1");

	printk("-------^v^-------\n");
	printk("-led drv v1 exit-\n");
}

//申明装载入口函数和卸载入口函数
module_init(led_drv_v1_init);
module_exit(led_drv_v1_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");