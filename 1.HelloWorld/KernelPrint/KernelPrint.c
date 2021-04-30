//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ide.h>

static char readbuf[100];  // 读缓冲区
static char writebuf[100]; // 写缓冲区
static char message[] = {"This message comes from kernel."};

static int drive_major; //设备号
static struct class *KernelPrint_cls;

static int KernelPrint_open(struct inode *inode, struct file *filp) //打开函数
{
	//本DEMO无需申请资源，此处留白
	printk("-KernelPrint open-\n");
	return 0;
}

static ssize_t KernelPrint_read(struct file *filp, char __user *buf, size_t count, loff_t *fops) //用户读取，内核发送信息
{
	int flag = 0;
	memcpy(readbuf, message, sizeof(message)); //使用memcpy将内核中要发送的内容写入读缓冲区
	flag = copy_to_user(buf, readbuf, count);  //使用copy_to_user函数将读缓冲区的内容发送到用户态
	if (flag == 0)							   //返回0成功，否则失败
	{
		printk("Kernel send data success!\n");
	}
	else
	{
		printk("Kernel send data failed!\n");
	}
	printk("-KernelPrint read-\n");
	return 0;
}

static ssize_t KernelPrint_write(struct file *filp, const char __user *buf, size_t count, loff_t *fops) //用户发送，内核读取信息并打印
{
	int flag = 0;
	flag = copy_from_user(writebuf, buf, count); //使用copy_from_user读取用户态发送过来的数据
	if (flag == 0)
	{
		printk(KERN_CRIT "Kernel receive data: %s\n", writebuf);
	}
	else
	{
		printk("Kernel receive data failed!\n");
	}
	printk("-KernelPrint write-\n");
	return 0;
}

static int KernelPrint_release(struct inode *inode, struct file *filp) //释放设备
{
	//由于open函数并没有占用什么资源，因此无需释放
	printk("-KernelPrint release-\n");
	return 0;
}

static struct file_operations drive_fops = {
	.owner = THIS_MODULE,
	.open = KernelPrint_open,
	.read = KernelPrint_read,
	.write = KernelPrint_write,
	.release = KernelPrint_release,
};

//装载入口函数
static __init int KernelPrint_init(void)
{
	printk("-------^v^-------\n");
	printk("-KernelPrint init-\n");

	//申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	drive_major = register_chrdev(0, "KernelPrint", &drive_fops);
	if (drive_major < 0) //判断是否申请成功
	{
		printk("register chrdev faile!\n");
		return drive_major;
	}
	else
		printk("register chrdev ok!\n");

	//自动创建设备节点
	//创建设备的类别
	//参数1----设备的拥有者，当前模块，直接填THIS_MODULE
	//参数2----设备类别的名字，自定义
	//返回值：类别结构体指针，其实就是分配了一个结构体空间
	KernelPrint_cls = class_create(THIS_MODULE, "KernelPrint_class");
	printk("class create ok!\n");

	//创建设备
	//参数1----设备对应的类别
	//参数2----当前设备的父类，直接填NULL
	//参数3----设备节点关联的设备号
	//参数4----私有数据直接填NULL
	//参数5----设备节点的名字
	device_create(KernelPrint_cls, NULL, MKDEV(drive_major, 0), NULL, "KernelPrint_%d", 0);
	printk("device create ok!\n");

	return 0;
}

//卸载入口函数
static __exit void KernelPrint_exit(void)
{
	device_destroy(KernelPrint_cls, MKDEV(drive_major, 0)); //删除设备
	class_destroy(KernelPrint_cls);							//删除类
	unregister_chrdev(drive_major, "KernelPrint");			//注销主设备号
	printk("-------^v^-------\n");
	printk("-KernelPrint exit-\n");
}

//申明装载入口函数和卸载入口函数
module_init(KernelPrint_init);
module_exit(KernelPrint_exit);

//添加各类信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");