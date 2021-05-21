#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/watchdog.h>

int main(int argc, char *argv[])
{
	int fd = -1, cmd = -1, timeout = 0;

	//打开看门狗
	fd = open("/dev/watchdog1", O_RDWR);
	if (fd < 0)
	{
		printf("Error: Failed to open /dev/watchdog1\n");
		return -1;
	}
	//使能看门狗
	ioctl(fd, WDIOC_SETOPTIONS, WDIOS_ENABLECARD);
	switch (atoi(argv[1]))
	{
	case 0:
		//获取超时时间
		timeout = atoi(argv[2]);
		printf("Timeout = %ds\n", timeout);
		//设置超时
		ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
		break;
	case 1:
		//喂狗
		ioctl(fd, WDIOC_KEEPALIVE, NULL);
		break;
	default:
		break;
	}

	close(fd);
	return 0;
}