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
	int fd = 0, a = 0, ret = 0, i = 0;
	struct timeval timeout;

	//设置超时，上面为秒，下面为微秒
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	fd = open("/dev/nio_device", O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		perror("open fail!\n");
		exit(1);
	}

	while (i < 10)
	{
		//初始化可读性
		FD_ZERO(&readfds);
		//设置可读性
		FD_SET(fd, &readfds);
		ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch (ret)
		{
		case 0: //超时
			printf("%d.Time out.\n", i);
			break;
		case 1: //成功读取按键
			read(fd, &a, sizeof(a));
			printf("%d.but1 value %d\n", i, a);
			break;
		default: //其他错误
			printf("Unknow error.\n");
			break;
		}

		//sleep(1);
		i++;
	}
	close(fd);
	return 0;
}
