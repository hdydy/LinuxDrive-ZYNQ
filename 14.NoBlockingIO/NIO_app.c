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

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	while (1)
	{
		fd = open("/dev/nio_device", O_RDWR);
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		ret = select(fd + 1, &readfds, NULL, NULL, &timeout);
		switch (ret)
		{
		case 0:
			printf("Time out.\n");
			break;
		case 1:
			read(fd, &a, sizeof(a));
			printf("but1 value %d\n", a);
			break;
		default:
			printf("Unknow error.\n");
			break;
		}
		close(fd);
		sleep(1);
	}
	return 0;
}