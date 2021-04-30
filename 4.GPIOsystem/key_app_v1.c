#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(void)
{
	int fd = 0;
	int i = 0;
	int a[4] = {0};

	fd = open("/dev/mykey0", O_RDWR);
	if (fd < 0)
	{
		perror("open fail!\n");
		exit(1);
	}

	while (1)
	{
		read(fd, a, sizeof(a));
		printf("but1 value %d\n", a[0]);
		printf("but2 value %d\n", a[1]);
		printf("but3 value %d\n", a[2]);
		printf("but4 value %d\n", a[3]);
		sleep(3);
	}

	close(fd);

	return 0;
}