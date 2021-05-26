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
	printf("%s r     : read at24c02 addresss 0x100\n", str);
	printf("%s w val : write at24c02 addresss 0x100\n", str);
}

int main(int argc, char *argv[])
{
	int i;
	int fd;
	unsigned char val; //字节

	unsigned int register_addr = 0x100; //片内地址

	//存储读写数据的数组
	char wbuf[9] = {0};
	char rbuf[8] = {0};

	//判读执行文件时是否带有一个以上的参数，没有则提示
	if (argc < 2)
	{
		print_usage(argv[0]);
		exit(1);
	}

	/*打开设备文件*/
	fd = open("/dev/EEPROM_device0", O_RDWR);
	if (fd < 0)
	{
		perror("open failed");
		exit(1);
	}

	if (strcmp(argv[1], "r") == 0)
	{
		//确定读哪个位置，读操作时先写片内地址
		if (write(fd, &register_addr, 1) < 1)
		{
			perror("write failed");
			exit(1);
		}

		//将EEPROM中的数据读到rbuf中
		if (read(fd, rbuf, 8) < 0)
		{
			perror("read failed");
			exit(1);
		}
		else
		{
			for (i = 0; i < 8; i++)
			{
				printf("rbuf[%d] = 0x%c ", i, rbuf[i]);
				if (i == 3)
					printf("\n");
			}
			printf("\n");
		}
	}
	else if ((strcmp(argv[1], "w") == 0) && (argc == 3))
	{
		char num[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
		int x = strtoul(argv[2], NULL, 0);
		if (x < 0 || x > 15)
		{
			perror("write overflow");
			close(fd);
			exit(1);
		}
		else
			val = num[x];

		//先写片内地址
		wbuf[0] = register_addr; // 片内地址0x08
		wbuf[1] = val;
		printf("val=%d\n", wbuf[1]);

		for (i = 2; i < 9; i++)
			wbuf[i] = '9';

		//连续向fd文件中写入8个8字符
		if (write(fd, wbuf, 9) < 0)
		{
			perror("write failed");
			close(fd);
			exit(1);
		}

		printf("write data ok!\n");
	}

	close(fd);
	return 0;
}
