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
	int a = 0;

	fd = open("/dev/misc_device", O_RDWR);
	if (fd < 0)
	{
		perror("open fail!\n");
		exit(1);
	}

	while (1)
	{
		read(fd, &a, sizeof(a));
		printf("but1 value %d\n", a);
	}

	close(fd);

	return 0;
}