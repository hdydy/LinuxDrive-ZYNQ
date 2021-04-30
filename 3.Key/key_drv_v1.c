//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>

static int key_major;
static struct class *key_cls;

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
#define GPIO_PIN_42 (unsigned int *)((unsigned long)iou_slcr + 0x000000A8)
#define GPIO_PIN_40 (unsigned int *)((unsigned long)iou_slcr + 0x000000A0)
#define GPIO_PIN_44 (unsigned int *)((unsigned long)iou_slcr + 0x000000B0)
#define GPIO_PIN_36 (unsigned int *)((unsigned long)iou_slcr + 0x00000090)

/* gpio 控制电流0*/
#define IOU_SLCR_BANK1_CTRL0 (unsigned int *)((unsigned long)iou_slcr + 0x00000154)
/* gpio 控制电流1*/
#define IOU_SLCR_BANK1_CTRL1 (unsigned int *)((unsigned long)iou_slcr + 0x00000158)
/* gpio 输入引脚选择Schmitt或CMOS*/
#define IOU_SLCR_BANK1_CTRL3 (unsigned int *)((unsigned long)iou_slcr + 0x00000158)
/* gpio 设置上下拉*/
#define IOU_SLCR_BANK1_CTRL4 (unsigned int *)((unsigned long)iou_slcr + 0x00000160)
/* gpio 使能上下拉*/
#define IOU_SLCR_BANK1_CTRL5 (unsigned int *)((unsigned long)iou_slcr + 0x00000164)
/* gpio 引脚速率选择*/
#define IOU_SLCR_BANK1_CTRL6 (unsigned int *)((unsigned long)iou_slcr + 0x00000168)

/* gpio 方向寄存器 */
#define GPIO_DIRM_1 (unsigned int *)((unsigned long)gpio_addr + 0x00000244)
/* gpio 使能寄存器 */
#define GPIO_OEN_1 (unsigned int *)((unsigned long)gpio_addr + 0x00000248)
/* gpio 输入数据寄存器 */
#define GPIO_DATA_R0_1 (unsigned int *)((unsigned long)gpio_addr + 0x00000064)

int key_open(struct inode *inode, struct file *filp)
{
	printk("-key open-\n");

	return 0;
}
ssize_t key_read(struct file *filp, char __user *buf, size_t count, loff_t *fops)
{
	int flag = 0;
	int value[4] = {0};
	printk("-key read 0x%0x-\n", *GPIO_DATA_R0_1);
	value[0] = ioread32(GPIO_DATA_R0_1) % (1 << 17) / (1 << 16);
	value[1] = ioread32(GPIO_DATA_R0_1) % (1 << 15) / (1 << 14);
	value[2] = ioread32(GPIO_DATA_R0_1) % (1 << 19) / (1 << 18);
	value[3] = ioread32(GPIO_DATA_R0_1) % (1 << 11) / (1 << 10);
	flag = copy_to_user(buf, value, count);
	if (flag != 0) //返回0成功，否则失败
	{
		printk("Kernel send data failed!\n");
	}

	return 0;
}
int key_close(struct inode *inode, struct file *filp)
{
	printk("-key close-\n");
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
	printk("-------^v^-------\n");
	printk("-key drv v1 init-\n");
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

	//④、硬件初始化
	//内存映射
	gpio_addr = ioremap(GPIO_BASE_ADDR, GPIO_BASE_SIZE);
	iou_slcr = ioremap(IOU_SLCR_ADDR, IOU_SLCR_SIZE);

	/* MIO42 MIO40 MIO44 MIO36 设置成GPIO */
	iowrite32((ioread32(GPIO_PIN_42) & 0x0), GPIO_PIN_42);
	iowrite32((ioread32(GPIO_PIN_40) & 0x0), GPIO_PIN_40);
	iowrite32((ioread32(GPIO_PIN_44) & 0x0), GPIO_PIN_44);
	iowrite32((ioread32(GPIO_PIN_36) & 0x0), GPIO_PIN_36);

	/* MIO7 MIO12设置输出驱动电流大小 */
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL0) | 0x3FFFFFF), IOU_SLCR_BANK1_CTRL0);
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL1) & 0x0), IOU_SLCR_BANK1_CTRL1);

	/* 选择引脚是Schmitt还是CMOS */
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL3) & 0x0), IOU_SLCR_BANK1_CTRL3);

	/* 输出管脚上下拉及使能 */
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL4) | 0x3FFFFFF), IOU_SLCR_BANK1_CTRL4);
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL5) | 0x3FFFFFF), IOU_SLCR_BANK1_CTRL5);

	/* MIO速率的选择 */
	iowrite32((ioread32(IOU_SLCR_BANK1_CTRL6) & 0x0), IOU_SLCR_BANK1_CTRL6);

	/* MIO42 MIO40 MIO44 MIO36 设置成输入 */
	iowrite32((ioread32(GPIO_DIRM_1) & 0xFFFABBFF), GPIO_DIRM_1);
	/* MIO42 MIO40 MIO44 MIO36 使能 */
	iowrite32((ioread32(GPIO_OEN_1) & 0xFFFABBFF), GPIO_OEN_1);

	return 0;
}

static __exit void key_drv_v1_exit(void)
{
	printk("-------^v^-------\n");
	printk("-key drv v1 exit-\n");

	//内存释放
	iounmap(gpio_addr);
	iounmap(iou_slcr);
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