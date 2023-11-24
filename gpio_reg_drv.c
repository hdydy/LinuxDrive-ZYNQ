/**
 * @file gpio_reg_drv.c
 * @author
 * @brief The drive to control GPIO base on register.
 * @version 0.1
 * @date 2023-11-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include "gpio_reg.h"

static int gpio_reg_major;
static struct class *gpio_reg_class;

/*
 * GPIO status.
 * The first bit records the initialization status. 0 means uninitialized, 1 means initialized.
 * The second bit records the direction. 0 means input, 1 means output.
 */
static unsigned int IO_STAT_LIST[174][2] = {0};

/* Pointer of the gpio memory map address */
static unsigned long *gpio_base = NULL;
static unsigned long *iou_slcr = NULL;

/* gpio register base address */
#define GPIO_BASE_ADDR 0xFF0A0000
#define IOU_SLCR_ADDR 0xFF180000
/* gpio register size */
#define GPIO_BASE_SIZE 0x368
#define IOU_SLCR_SIZE 0x714

/* Register offsets for the GPIO device */
/* 0 <= BANK <= 5 */
/* LSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_LSW_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x000 + (8 * BANK))
/* MSW Mask & Data -WO */
#define ZYNQ_GPIO_DATA_MSW_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x004 + (8 * BANK))
/* Data Register-RW */
#define ZYNQ_GPIO_DATA_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x040 + (4 * BANK))
#define ZYNQ_GPIO_DATARO_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x060 + (4 * BANK))
/* Direction mode reg-RW */
#define ZYNQ_GPIO_DIRM_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x204 + (0x40 * BANK))
/* Output enable reg-RW */
#define ZYNQ_GPIO_OEN_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x208 + (0x40 * BANK))
/* Interrupt mask reg-RO */
#define ZYNQ_GPIO_INTMASK_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x20C + (0x40 * BANK))
/* Interrupt enable reg-WO */
#define ZYNQ_GPIO_INTEN_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x210 + (0x40 * BANK))
/* Interrupt disable reg-WO */
#define ZYNQ_GPIO_INTDIS_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x214 + (0x40 * BANK))
/* Interrupt status reg-RO */
#define ZYNQ_GPIO_INTSTAT_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x218 + (0x40 * BANK))
/* Interrupt type reg-RW */
#define ZYNQ_GPIO_INTTYPE_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x21C + (0x40 * BANK))
/* Interrupt polarity reg-RW */
#define ZYNQ_GPIO_INTPOLARITY_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x220 + (0x40 * BANK))
/* Interrupt on any, reg-RW */
#define ZYNQ_GPIO_INTANY_OFFSET(BANK) (unsigned int *)((unsigned long)gpio_base + 0x224 + (0x40 * BANK))

/* IOP System-level Control */
/* MIO Device Pin Multiplexer Controls. 0 <= i <= 77 */
#define GPIO_PIN_(i) (unsigned int *)((unsigned long)iou_slcr + 4 * i)
/* 0 <= BANK <= 2 */
/* Drive 0 control. */
#define ZYNQ_GPIO_CTRL0(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x138 + (0x1C * BANK))
/* Drive 1 control. */
#define ZYNQ_GPIO_CTRL1(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x13C + (0x1C * BANK))
/* CMOS input type control. */
#define ZYNQ_GPIO_CTRL3(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x140 + (0x1C * BANK))
/* Pull up/down enable. */
#define ZYNQ_GPIO_CTRL4(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x144 + (0x1C * BANK))
/* Pull up/down select. */
#define ZYNQ_GPIO_CTRL5(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x148 + (0x1C * BANK))
/* Output slew rate select. */
#define ZYNQ_GPIO_CTRL6(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x14C + (0x1C * BANK))
/* Voltage detection. */
#define ZYNQ_GPIO_STATUS(BANK) (unsigned int *)((unsigned long)iou_slcr + 0x150 + (0x1C * BANK))

/**
 * @brief Get the bank number of the pin and its pin number within the bank.
 *
 * @param pin_num The pin number of the GPIO.
 * @param bank_num The bank number to which the GPIO belongs.
 * @param bank_pin_num The pin number of the GPIO within the bank.
 */
static inline void GET_BANK_PIN(unsigned long pin_num, unsigned int *bank_num, unsigned int *bank_pin_num)
{
	if (0 <= pin_num && pin_num <= 25)
	{
		*bank_num = 0;
		*bank_pin_num = pin_num;
		return;
	}
	if (26 <= pin_num && pin_num <= 51)
	{
		*bank_num = 1;
		*bank_pin_num = pin_num - 26;
		return;
	}
	if (52 <= pin_num && pin_num <= 77)
	{
		*bank_num = 2;
		*bank_pin_num = pin_num - 52;
		return;
	}
	if (78 <= pin_num && pin_num <= 109)
	{
		*bank_num = 3;
		*bank_pin_num = pin_num - 78;
		return;
	}
	if (110 <= pin_num && pin_num <= 141)
	{
		*bank_num = 4;
		*bank_pin_num = pin_num - 110;
		return;
	}
	if (142 <= pin_num && pin_num <= 173)
	{
		*bank_num = 5;
		*bank_pin_num = pin_num - 142;
		return;
	}

	/* default */
	pr_warn("invalid GPIO pin number: %lu", pin_num);
	*bank_num = 0;
	*bank_pin_num = 0;
}

/**
 * @brief Initialize the MIO pin. The pin number should between 0 and 77.
 *
 * @param pin_num The pin number need to be initialized.
 * @param bank_num The bank number of the pin.
 * @param bank_pin_num The pin number of the pin within the bank.
 * @return 0 - Initialization successful.
 */
static inline int MIO_INIT(unsigned long pin_num, unsigned int bank_num, unsigned int bank_pin_num)
{
	// MIO Device Pin Multiplexer Controls.
	iowrite32((ioread32(GPIO_PIN_(pin_num)) & 0x0), GPIO_PIN_(pin_num));

	// Control the output drive strength of MIO pin
	iowrite32((ioread32(ZYNQ_GPIO_CTRL0(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL0(bank_num));
	iowrite32((ioread32(ZYNQ_GPIO_CTRL1(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL1(bank_num));

	//  CMOS input type control.
	iowrite32((ioread32(ZYNQ_GPIO_CTRL3(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_CTRL3(bank_num));

	// Pull up/down select.
	iowrite32((ioread32(ZYNQ_GPIO_CTRL4(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL4(bank_num));
	// Pull up/down enable.
	iowrite32((ioread32(ZYNQ_GPIO_CTRL5(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL5(bank_num));

	// Output slew rate select.
	iowrite32((ioread32(ZYNQ_GPIO_CTRL6(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_CTRL6(bank_num));
	return 0;
}

/**
 * @brief Initialize MIO and EMIO.
 *
 * @param pin_num The pin number of the GPIO.
 * @param direction GPIO direction, 0 sets input, 1 sets output.
 * @return  0 - Success. Others - Fail.
 */
static inline int GPIO_INIT(unsigned long pin_num, unsigned long direction)
{
	int ret = 0;
	unsigned int bank_num, bank_pin_num;

	/*
	 * On zynq bank 0 pins 7 and 8 are special and cannot be used as inputs.
	 */
	if (direction == 0 && (pin_num == 7 || pin_num == 8))
		return -EINVAL;

	GET_BANK_PIN(pin_num, &bank_num, &bank_pin_num);
	if (0 <= pin_num && pin_num <= 77)
		ret = MIO_INIT(pin_num, bank_num, bank_pin_num);
	if (value)
	{
		// Set direction to OUT
		iowrite32((ioread32(ZYNQ_GPIO_DIRM_OFFSET(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_DIRM_OFFSET(bank_num));
		// Output enable
		iowrite32((ioread32(ZYNQ_GPIO_OEN_OFFSET(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_OEN_OFFSET(bank_num));
	}
	else
	{
		// Set direction to IN
		iowrite32((ioread32(ZYNQ_GPIO_DIRM_OFFSET(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_DIRM_OFFSET(bank_num));
		// Output disable
		iowrite32((ioread32(ZYNQ_GPIO_OEN_OFFSET(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_OEN_OFFSET(bank_num));
	}
	IO_STAT_LIST[pin_num][0] = 1;
	IO_STAT_LIST[pin_num][1] = direction;

	return ret;
}

/**
 * @brief Get value of the pin.
 *
 * @param pin_num The pin number of the GPIO.
 * @param value The value of the GPIO.
 * @return 0 - Success. Others - Fail.
 */
static inline int GPIO_GET_VALUE(unsigned int pin_num, unsigned long *value)
{
	unsigned int bank_num, bank_pin_num;
	if (IO_STAT_LIST[pin_num][0] && (IO_STAT_LIST[pin_num][1] != 0))
		return -EINVAL;
	GET_BANK_PIN(pin_num, &bank_num, &bank_pin_num);
	*value = ioread32(ZYNQ_GPIO_DATARO_OFFSET(bank_num)) % (1 << (bank_pin_num + 1)) / (1 << bank_pin_num);
	return 0;
}

/**
 * @brief Set value of the pin.
 *
 * @param pin_num The pin number of the GPIO.
 * @param value The value needs to be set.
 * @return 0 - Success. Others - Fail.
 */
static inline int GPIO_SET_VALUE(unsigned int pin_num, unsigned long value)
{
	unsigned int bank_num, bank_pin_num;
	if (IO_STAT_LIST[pin_num][0] && (IO_STAT_LIST[pin_num][1] != 1))
		return -EINVAL;
	GET_BANK_PIN(pin_num, &bank_num, &bank_pin_num);
	if (value)
		iowrite32((ioread32(ZYNQ_GPIO_DATA_OFFSET(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_DATA_OFFSET(bank_num));
	else
		iowrite32((ioread32(ZYNQ_GPIO_DATA_OFFSET(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_DATA_OFFSET(bank_num));
	return 0;
}

/**
 * @brief Release the pins initialized.
 *
 */
static inline void GPIO_RESET(void)
{
	int i = 0;
	unsigned int bank_num, bank_pin_num;
	for (i = 0; i < 174; i++)
	{
		if (IO_STAT_LIST[i][0] == 1)
		{
			GET_BANK_PIN(i, &bank_num, &bank_pin_num);
			if (i < 78)
			{
				iowrite32((ioread32(GPIO_PIN_(i)) & 0x0), GPIO_PIN_(i));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL0(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL0(bank_num));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL1(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_CTRL1(bank_num));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL3(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_CTRL3(bank_num));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL4(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL4(bank_num));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL5(bank_num)) | (1 << bank_pin_num)), ZYNQ_GPIO_CTRL5(bank_num));
				iowrite32((ioread32(ZYNQ_GPIO_CTRL6(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_CTRL6(bank_num));
			}
			iowrite32((ioread32(ZYNQ_GPIO_DIRM_OFFSET(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_DIRM_OFFSET(bank_num));
			iowrite32((ioread32(ZYNQ_GPIO_OEN_OFFSET(bank_num)) & ~(1 << bank_pin_num)), ZYNQ_GPIO_OEN_OFFSET(bank_num));
			IO_STAT_LIST[i][0] = 0;
			IO_STAT_LIST[i][1] = 0;
		}
	}
}

static int gpio_reg_open(struct inode *inode, struct file *filp)
{
	pr_info("Initialize gpio.\n");
	// Memory map
	gpio_base = ioremap(GPIO_BASE_ADDR, GPIO_BASE_SIZE);
	iou_slcr = ioremap(IOU_SLCR_ADDR, IOU_SLCR_SIZE);
	return 0;
}

static long gpio_reg_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	unsigned long value = 0;

	if (_IOC_TYPE(cmd) != CMD_IOC_INIT_MAGIC && _IOC_TYPE(cmd) != CMD_IOC_I_MAGIC && _IOC_TYPE(cmd) != CMD_IOC_O_MAGIC)
	{
		pr_err("%s: command type [%c] error.\n", __func__, _IOC_TYPE(cmd));
		return -ENOTTY;
	}
	if ((_IOC_TYPE(cmd) == CMD_IOC_INIT_MAGIC || _IOC_TYPE(cmd) == CMD_IOC_O_MAGIC) && (arg != 0 && arg != 1))
	{
		pr_err("%s: command arg [%ld] error.\n", __func__, arg);
		return -ENOTTY;
	}

	switch (_IOC_TYPE(cmd))
	{
	case CMD_IOC_INIT_MAGIC:
		ret = GPIO_INIT(_IOC_NR(cmd), arg);
		break;
	case CMD_IOC_I_MAGIC:
		ret = GPIO_GET_VALUE(_IOC_NR(cmd), &value);
		ret = put_user(value, (int __user *)arg);
		break;
	case CMD_IOC_O_MAGIC:
		ret = GPIO_SET_VALUE(_IOC_NR(cmd), arg);
		break;
	default:
		ret = -EFAULT;
		break;
	}
	return ret;
}

static int gpio_reg_release(struct inode *inode, struct file *filp)
{
	GPIO_RESET();
	iounmap(gpio_base);
	iounmap(iou_slcr);
	pr_info("Release gpio.\n");
	return 0;
}

const struct file_operations gpio_reg_fops = {
	.owner = THIS_MODULE,
	.open = gpio_reg_open,
	.release = gpio_reg_release,
	.unlocked_ioctl = gpio_reg_ioctl,
};

static __init int gpio_reg_drv_init(void)
{
	// Apply for major.
	gpio_reg_major = register_chrdev(0, "gpio_reg_drv", &gpio_reg_fops);
	if (gpio_reg_major < 0)
	{
		pr_err("Register chrdev failed.\n");
		return gpio_reg_major;
	}

	// Create a class.
	gpio_reg_class = class_create(THIS_MODULE, "gpio_reg_class");

	// Create a device.
	device_create(gpio_reg_class, NULL, MKDEV(gpio_reg_major, 0), NULL, "gpio_reg_drv");

	pr_info("gpio_reg_drv init\n");

	return 0;
}

static __exit void gpio_reg_drv_exit(void)
{
	device_destroy(gpio_reg_class, MKDEV(gpio_reg_major, 0));
	class_destroy(gpio_reg_class);
	unregister_chrdev(gpio_reg_major, "gpio_reg_drv");

	pr_info("gpio_reg_drv exit\n");
}

module_init(gpio_reg_drv_init);
module_exit(gpio_reg_drv_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("makai@milianke.com");