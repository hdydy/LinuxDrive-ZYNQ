//添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/atomic.h>
#include <linux/input.h>

#define ZYNQMP_GPIO_NR_GPIOS 174
#define MIO_PIN_42 (ARCH_NR_GPIOS - ZYNQMP_GPIO_NR_GPIOS + 42)

//设置一个设备全局变量
struct input_device
{
	dev_t devno;				 //设备号
	struct cdev cdev;			 //字符设备
	struct class *class;		 //设备类
	struct device *device;		 //设备
	atomic64_t state;			 //原子变量，此处用作按键状态
	unsigned int irq;			 //中断
	struct work_struct key_work; //工作队列
	wait_queue_head_t waitqueue; //等待队列
	struct input_dev *inputdev;	 //input_dev结构体
	unsigned char code;			 //input事件码
} input_dev;					 //定义一个全局的设备结构体

//中断下半部
static void key_do_work(struct work_struct *work)
{
	//printk(KERN_CRIT "key dowm irq headler!\n");
	//储存按键状态
	int value;

	//读取按键值
	value = gpio_get_value(MIO_PIN_42);
	//printk(KERN_CRIT "value = %d\n", value);

	input_report_key(input_dev.inputdev, input_dev.code, !value);
	input_sync(input_dev.inputdev);

	//解锁
	atomic_set(&input_dev.state, 0);
}

//中断上半部
static irqreturn_t key_irq_handler(int irq, void *dev_id)
{
	//printk(KERN_CRIT "key irq handler!\n");
	if (!atomic64_read(&input_dev.state)) //读取锁的状态
		atomic_set(&input_dev.state, 1);  //把原子变量置1, 上锁
	else
		return -EBUSY; //若检测到已上锁，则返回设备忙
	//唤起key_work工作队列
	schedule_work(&input_dev.key_work);
	return IRQ_HANDLED;
}

const struct file_operations input_fops = {};

//实现装载入口函数和卸载入口函数
static __init int input_drv_init(void)
{
	int ret = 0;

	printk("----^v^-----interrupt drv v1 init\n");
	/*
	//申请主设备号
	ret = alloc_chrdev_region(&input_dev.devno, 0, 1, "input_device");
	if (ret < 0)
	{
		printk("alloc_chrdev_region fail!\n");
		return ret;
	}

	//创建设备类
	input_dev.class = class_create(THIS_MODULE, "input_class");
	if (IS_ERR(input_dev.class))
	{
		printk("class_create fail!\n");
		return 0;
	}

	//创建设备
	input_dev.device = device_create(input_dev.class, NULL, input_dev.devno, NULL, "input_device");
	if (IS_ERR(input_dev.device))
	{
		printk("device_create fail!\n");
		return 0;
	}

	//设备初始化
	cdev_init(&input_dev.cdev, &input_fops);
	input_dev.cdev.owner = THIS_MODULE;
	//向系统注册一个字符设备
	cdev_add(&input_dev.cdev, input_dev.devno, 1);
	*/

	atomic_set(&input_dev.state, 0);

	/* 设置事件码为 KEY_0 */
	input_dev.code = KEY_0;

	/* 申请 input_dev 结构体变量 */
	input_dev.inputdev = input_allocate_device();

	input_dev.inputdev->name = "input_device";
	/* 设置按键事件 */
	__set_bit(EV_KEY, input_dev.inputdev->evbit);
	/* 设置按键重复事件 */
	__set_bit(EV_REP, input_dev.inputdev->evbit);
	/* 设置按键事件码 */
	__set_bit(KEY_0, input_dev.inputdev->keybit);

	/* 注册 input_dev 结构体变量 */
	ret = input_register_device(input_dev.inputdev);
	if (ret)
	{
		printk("register input device failed\r\n");
		return ret;
	}

	//获取中断号
	input_dev.irq = gpio_to_irq(MIO_PIN_42);
	printk("irq = %d\n", input_dev.irq);

	//申请中断
	ret = request_irq(input_dev.irq, key_irq_handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, "input_key", NULL);
	if (ret < 0)
	{
		printk("request_irq fail!\n");
		return ret;
	}

	//在key_work工作队列中添加一个key_do_work任务
	INIT_WORK(&input_dev.key_work, key_do_work);
	return 0;
}

//实现装载入口函数和卸载入口函数
static __exit void input_drv_exit(void)
{
	printk("----^v^-----interrupt drv v1 exit\n");

	cancel_work_sync(&input_dev.key_work);
	//释放irq
	free_irq(input_dev.irq, NULL);
	/*
	//删除设备
	device_destroy(input_dev.class, input_dev.devno);
	//删除类
	class_destroy(input_dev.class);
	//注销主设备号
	unregister_chrdev(input_dev.devno, "input_device");
	*/
	/* 注销 input_dev 结构体变量 */
	input_unregister_device(input_dev.inputdev);
	/* 释放 input_dev 结构体变量 */
	input_free_device(input_dev.inputdev);
}

//申明装载入口函数和卸载入口函数
module_init(input_drv_init);
module_exit(input_drv_exit);

//添加GPL协议
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");