/**
 * @file gpio_reg.h
 * @author makai (makai@milianke.com)
 * @brief The header of gpio_reg_drv.c. i is the pin number of a GPIO. 
 *        In CMD_SET_DIRECTION(i), int should be the direction of a GPIO. 
 *        0 means input, 1 means output.
 *        In CMD_GET_VALUE(i) or CMD_SET_VALUE(i), int should be the voltage level.
 *        0 means low voltage level, 1 means high voltage level.
 * @version 0.1
 * @date 2023-11-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef _GPIO_REG_H_
#define _GPIO_REG_H_

#define CMD_IOC_INIT_MAGIC 'A'
#define CMD_IOC_I_MAGIC 'I'
#define CMD_IOC_O_MAGIC 'O'
#define CMD_SET_DIRECTION(i) _IOW(CMD_IOC_INIT_MAGIC, i, int)
#define CMD_GET_VALUE(i) _IOR(CMD_IOC_I_MAGIC, i, int)
#define CMD_SET_VALUE(i) _IOW(CMD_IOC_O_MAGIC, i, int)

#endif