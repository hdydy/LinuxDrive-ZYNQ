//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_42 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 42)
#define MIO_PIN_40 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 40)
#define MIO_PIN_44 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 44)
#define MIO_PIN_36 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 36)

static int interrupt_major;
static int ret = 0;
static int irq1, irq2, irq3, irq4;

static struct class *interrupt_cls;

struct work_struct key_work_0;
struct work_struct key_work_1;
struct work_struct key_work_2;
struct work_struct key_work_3;

/*中断下半部*/
void key_do_work_0(struct work_struct *work)
{
	printk(KERN_CRIT "key1 dowm irq headler!\n");
}
void key_do_work_1(struct work_struct *work)
{
	printk(KERN_CRIT "key2 dowm irq headler!\n");
}
void key_do_work_2(struct work_struct *work)
{
	printk(KERN_CRIT "key3 dowm irq headler!\n");
}
void key_do_work_3(struct work_struct *work)
{
	printk(KERN_CRIT "key4 dowm irq headler!\n");
}

/*中断上半部*/
static irqreturn_t key1_irq_handler(int irqe, void *dev_id)
{
	printk(KERN_CRIT "key1 irq handler!\n");
	//唤起key_work工作队列
	schedule_work(&key_work_0);
	return IRQ_HANDLED;
}

static irqreturn_t key2_irq_handler(int irqe, void *dev_id)
{
	printk(KERN_CRIT "key2 irq handler!\n");
	//唤起key_work工作队列
	schedule_work(&key_work_1);
	return IRQ_HANDLED;
}
static irqreturn_t key3_irq_handler(int irqe, void *dev_id)
{
	printk(KERN_CRIT "key3 irq handler!\n");
	//唤起key_work工作队列
	schedule_work(&key_work_2);
	return IRQ_HANDLED;
}
static irqreturn_t key4_irq_handler(int irqe, void *dev_id)
{
	printk(KERN_CRIT "key4 irq handler!\n");
	//唤起key_work工作队列
	schedule_work(&key_work_3);
	return IRQ_HANDLED;
}

const struct file_operations interrupt_fops = {};

//4、实现装载入口函数和卸载入口函数
static __init int interrupt_drv_v1_init(void)
{
	printk("----^v^-----interrupt drv v1 init\n");
	//①、申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	interrupt_major = register_chrdev(0, "interrupt_drv_v1", &interrupt_fops);
	if (interrupt_major < 0)
	{
		printk("register chrdev faile!\n");
		return interrupt_major;
	}

	//②、自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	interrupt_cls = class_create(THIS_MODULE, "interrupt_class");

	//③、创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	device_create(interrupt_cls, NULL, MKDEV(interrupt_major, 0), NULL, "myinterrupt%d", 0);

	//⑤、获取中断号
	irq1 = gpio_to_irq(MIO_PIN_42);
	irq2 = gpio_to_irq(MIO_PIN_40);
	irq3 = gpio_to_irq(MIO_PIN_44);
	irq4 = gpio_to_irq(MIO_PIN_36);

	printk(" gpio irq1 = %d\n gpio irq2 = %d\n gpio irq3 = %d\n gpio irq4 = %d\n", irq1, irq2, irq3, irq4);

	//⑥、申请中断
	/*
	 * 参数1--中断号码---为哪个中断设置处理方法
	 * 参数2--中断的处理函数
	 * 参数3--中断的触发方式
	 * #define IRQF_TRIGGER_NONE    0x00000000 		无触发中断（采用默认的或之前设置的触发方式）
	 * #define IRQF_TRIGGER_RISING  0x00000001 		指定中断触发类型：上升沿有效
	 * #define IRQF_TRIGGER_FALLING 0x00000002		中断触发类型：下降沿有效
	 * #define IRQF_TRIGGER_HIGH    0x00000004		指定中断触发类型：高电平有效
	 * #define IRQF_TRIGGER_LOW     0x00000008		指定中断触发类型：低电平有效
	 * #define IRQF_TRIGGER_MASK   (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
	 * #define IRQF_TRIGGER_PROBE   0x00000010		触发式检测中断
	 * 参数4--中断的描述--自定义
	 * 参数5--传递给中断处理函数的参数
	 * 返回值--出错小于0 正确为0
	 */
	ret = request_irq(irq1, key1_irq_handler, IRQF_TRIGGER_RISING, "interrupt_key1", NULL);
	if (ret < 0)
	{
		printk("request irq1 fail!\n");
		return ret;
	}
	ret = request_irq(irq2, key2_irq_handler, IRQF_TRIGGER_RISING, "interrupt_key2", NULL);
	if (ret < 0)
	{
		printk("request irq2 fail!\n");
		return ret;
	}
	ret = request_irq(irq3, key3_irq_handler, IRQF_TRIGGER_RISING, "interrupt_key3", NULL);
	if (ret < 0)
	{
		printk("request irq3 fail!\n");
		return ret;
	}
	ret = request_irq(irq4, key4_irq_handler, IRQF_TRIGGER_RISING, "interrupt_key4", NULL);
	if (ret < 0)
	{
		printk("request irq4 fail!\n");
		return ret;
	}

	//在key_work工作队列中添加一个key_do_work任务
	INIT_WORK(&key_work_0, key_do_work_0);
	INIT_WORK(&key_work_1, key_do_work_1);
	INIT_WORK(&key_work_2, key_do_work_2);
	INIT_WORK(&key_work_3, key_do_work_3);
	return 0;
}

//4、实现装载入口函数和卸载入口函数
static __exit void interrupt_drv_v1_exit(void)
{
	printk("----^v^-----interrupt drv v1 exit\n");

	cancel_work_sync(&key_work_0);
	cancel_work_sync(&key_work_1);
	cancel_work_sync(&key_work_2);
	cancel_work_sync(&key_work_3);

	//释放irq
	free_irq(irq1, NULL);
	free_irq(irq2, NULL);
	free_irq(irq3, NULL);
	free_irq(irq4, NULL);

	//删除设备
	device_destroy(interrupt_cls, MKDEV(interrupt_major, 0));
	//删除类
	class_destroy(interrupt_cls);
	//注销主设备号
	unregister_chrdev(interrupt_major, "interrupt_drv_v1");
}

//2、申明装载入口函数和卸载入口函数
module_init(interrupt_drv_v1_init);
module_exit(interrupt_drv_v1_exit);

//3、添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");