#include <linux/i2c-dev.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//当执行app文件时没有带参数产生提示
void print_usage(char *str)
{
	printf("%s r     : read rtc addresss 0\n", str);
	printf("%s w val : write rtc addresss 0\n", str);
}

int main(int argc, char *argv[])
{
	int i;
	int fd;

	unsigned int register_addr = atoi(argv[2]);
	//片内地址

	//存储读写数据的数组
	char wbuf[10] = {0};
	char rbuf[10] = {0};
	int tmp[10] = {0};

	//判读执行文件时是否带有2个以上的参数，没有则提示
	if (argc < 3)
	{
		print_usage(argv[0]);
		exit(1);
	}

	/*打开设备文件*/
	fd = open("/dev/rtc_device0", O_RDWR);
	if (fd < 0)
	{
		perror("open /dev/rtc_device0 failed");
		exit(1);
	}

	printf("register_addr = %d\n", register_addr);
	if ((strcmp(argv[1], "r") == 0) && (argc >= 3))
	{

		i = 120;
		while (i--)
		{
			//确定读哪个位置，读操作时先写片内地址
			if (write(fd, &register_addr, 1) < 1)
			{
				perror("write failed");
				exit(1);
			}

			//将RTC中的数据读到rbuf中
			if (read(fd, rbuf, 10) < 1)
			{
				perror("write failed");
				exit(1);
			}

			printf("date %d-%d-%d\n", rbuf[6], rbuf[5], rbuf[4]);
			printf("week %d\n", rbuf[3]);
			printf("time %d:%d:%d\n", rbuf[2], rbuf[1], rbuf[0]);
			sleep(1);

			bzero(rbuf, sizeof(rbuf));
		}
	}
	else if ((strcmp(argv[1], "w") == 0) && (argc >= 3))
	{

		//  ./rtc_ds1337_app  w  0  date week|time
		//分别单独修改时间，星期几，日期
		if (strcmp(argv[3], "date") == 0)
		{
			wbuf[0] = 3;
			printf("date: ");

			scanf("%d %d %d %d", &tmp[4], &tmp[3], &tmp[2], &tmp[1]);
			wbuf[4] = tmp[4];
			wbuf[3] = tmp[3];
			wbuf[2] = tmp[2];
			wbuf[1] = tmp[1];
			printf("wbuf[0] = %d wbuf[1] = %d wbuf[2] = %d wbuf[3] = %d wbuf[4] = %d \n", wbuf[0], wbuf[1], wbuf[2], wbuf[3], wbuf[4]);

			//连续向fd文件中写入日期的数据
			if (write(fd, wbuf, 5) < 0)
			{
				perror("write failed");
				exit(1);
			}
		}

		if (strcmp(argv[3], "time") == 0)
		{
			wbuf[0] = 0;
			printf("time: ");
			scanf("%d %d %d", &tmp[3], &tmp[2], &tmp[1]);
			wbuf[3] = tmp[3];
			wbuf[2] = tmp[2];
			wbuf[1] = tmp[1];
			printf("wbuf[0] = %d wbuf[1] = %d wbuf[2] = %d wbuf[3] = %d\n", wbuf[0], wbuf[1], wbuf[2], wbuf[3]);

			//连续向fd文件中写入时间的数据
			if (write(fd, wbuf, 4) < 0)
			{
				perror("write failed");
				exit(1);
			}
		}
	}
	close(fd);
	return 0;
}