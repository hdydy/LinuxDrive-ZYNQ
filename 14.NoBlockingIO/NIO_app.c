#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int main(void)
{
	fd_set readfds;
	int fd = 0, a = 0, ret = 0;
	struct timeval timeout;

	//设置超时，上面为秒，下面为微秒
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	while (1)
	{
		//select函数会清空它所检测的socket描述符集合
		//每次调用select()之前都必须把socket描述符重新加入到待检测的集合中
		fd = open("/dev/nio_device", O_RDWR);
		if (fd < 0)
		{
			perror("open fail!\n");
			exit(1);
		}
		//初始化可读性
		FD_ZERO(&readfds);
		//设置可读性
		FD_SET(fd, &readfds);
		ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch (ret)
		{
		case 0: //超时
			printf("Time out.\n");
			break;
		case 1: //成功读取按键
			read(fd, &a, sizeof(a));
			printf("but1 value %d\n", a);
			break;
		default: //其他错误
			printf("Unknow error.\n");
			break;
		}
		close(fd);
		sleep(1);
	}
	return 0;
}