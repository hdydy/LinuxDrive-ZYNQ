//1、添加头文件
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ide.h>

static int drive_major; //设备号
static struct file_operations drive_fops = {};

//打印内容
int showmessage(void)
{
	printk(KERN_CRIT"__          __  _                            _          _                        _ _ ");
	printk(KERN_CRIT"\\ \\        / / | |                          | |        | |                      | | |");
	printk(KERN_CRIT" \\ \\  /\\  / /__| | ___ ___  _ __ ___   ___  | |_ ___   | | _____ _ __ _ __   ___| | |");
	printk(KERN_CRIT"  \\ \\/  \\/ / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\  | |/ / _ \\ '__| '_ \\ / _ \\ | |");
	printk(KERN_CRIT"   \\  /\\  /  __/ | (_| (_) | | | | | |  __/ | || (_) | |   <  __/ |  | | | |  __/ |_|");
	printk(KERN_CRIT"    \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/  |_|\\_\\___|_|  |_| |_|\\___|_(_)\n");
	return 0;
}

//装载入口函数
static __init int FirstDrive_init(void)
{
	printk("-------^v^-------\n");
	printk("-FirstDrive init-\n");

	//申请主设备号
	//参数1----需要的主设备号，>0静态分配， ==0自动分配
	//参数2----设备的描述 信息，体现在cat /proc/devices， 一般自定义
	//参数3----文件描述集合
	//返回值，小于0报错
	drive_major = register_chrdev(0, "FirstDrive", &drive_fops);
	if (drive_major < 0) //判断是否申请成功
	{
		printk("register chrdev faile!\n");
		return drive_major;
	}
	else
		printk("register chrdev ok!\n");

	showmessage();
	return 0;
}

//卸载入口函数
static __exit void FirstDrive_exit(void)
{
	unregister_chrdev(drive_major, "FirstDrive"); //注销主设备号
	printk("-------^v^-------\n");
	printk("-FirstDrive exit-\n");
}

//申明装载入口函数和卸载入口函数
module_init(FirstDrive_init);
module_exit(FirstDrive_exit);

//添加各类信息
MODULE_LICENSE("GPL");
MODULE_AUTHOR("msxbo");