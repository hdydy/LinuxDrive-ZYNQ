#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "signal.h"

static int fd = 0;

static void aio_async_func()
{
	int ret = 0, value = 0;
	ret = read(fd, &value, sizeof(value));
	if (ret < 0)
		printf("read fail.\n");
	if (value == 1)
	{
		printf("key value = %d\n", value);
		value = 0;
	}
}

int main(void)
{
	int flag = 0;
	fd = open("/dev/aio_device", O_RDWR);
	if (fd < 0)
	{
		perror("open fail!\n");
		exit(1);
	}

	//指定信号 SIGIO，并绑定处理函数
	signal(SIGIO, aio_async_func);
	//把当前线程指定为将接收信号的进程
	fcntl(fd, F_SETOWN, getpid());
	//获取当前线程状态
	flag = fcntl(fd, F_GETFD);
	//设置当前线程为 FASYNC 状态
	fcntl(fd, F_SETFL, flag | FASYNC);

	sleep(15);

	close(fd);
	return 0;
}
