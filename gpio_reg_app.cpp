#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "gpio_reg.h"

int main()
{
    int fd, value = 0;
    fd = open("/dev/gpio_reg_drv", O_RDWR, 666);
    if (fd < 0)
    {
        printf("can't open file gpio_reg_drv\n");
        return (-1);
    }
    if (1)
    {
        ioctl(fd, CMD_SET_DIRECTION(17), 1);
        for (int i = 0; i < 3; i++)
        {
            ioctl(fd, CMD_SET_VALUE(17), 0);
            sleep(1);
            ioctl(fd, CMD_SET_VALUE(17), 1);
            sleep(1);
        }
        ioctl(fd, CMD_SET_DIRECTION(23), 1);
        for (int i = 0; i < 3; i++)
        {
            ioctl(fd, CMD_SET_VALUE(23), 0);
            sleep(1);
            ioctl(fd, CMD_SET_VALUE(23), 1);
            sleep(1);
        }
    }
    else
    {
        ioctl(fd, CMD_SET_DIRECTION(45), 0);
        for (int i = 0; i < 3; i++)
        {
            ioctl(fd, CMD_GET_VALUE(45), &value);
            std::cout << "BUT1:" << value << std::endl;
            sleep(1);
            ioctl(fd, CMD_GET_VALUE(45), &value);
            std::cout << "BUT1:" << value << std::endl;
            sleep(1);
        }
        ioctl(fd, CMD_SET_DIRECTION(44), 0);
        for (int i = 0; i < 3; i++)
        {
            ioctl(fd, CMD_GET_VALUE(44), &value);
            std::cout << "BUT2:" << value << std::endl;
            sleep(1);
            ioctl(fd, CMD_GET_VALUE(44), &value);
            std::cout << "BUT2:" << value << std::endl;
            sleep(1);
        }
    }
    return 0;
}