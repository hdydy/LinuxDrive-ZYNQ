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

	fd = open("/dev/nio_device", O_RDWR);
	if (fd < 0)
	{
		perror("open fail!\n");
		exit(1);
	}

	FD_ZERO(&readfds);
	FD_SET(fd, &readfds);

	while (1)
	{
		ret = select(fd + 1, &readfds, NULL, NULL, NULL);
		switch (ret)
		{
		case 0:
			printf("Time out.");
			break;
		case 1:
			read(fd, &a, sizeof(a));
			printf("but1 value %d\n", a);
			break;
		default:
			printf("Unknow error.");
			break;
		}
	}

	close(fd);

	return 0;
}