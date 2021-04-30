#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int fd, retvalue;
	char *filename;
	char readbuf[100], writebuf[100];

	filename = argv[1];

	fd = open(filename, O_RDWR); //打开设备
	if (fd < 0)
	{
		printf("Can't open file %s\n", filename);
		return -1;
	}

	switch (*argv[2]) //对操作数进行解析
	{
	case 'r':
		if (argc != 3) //进行鲁棒性检查
		{
			printf("Unknow operation, use the formate: ./APPNAME /dev/DRIVENAME r to read date from kernel.\n");
			return -1;
		}
		retvalue = read(fd, readbuf, 100);
		if (retvalue < 0) //检查是否读取成功
		{
			printf("Read file %s failed!\n", filename);
		}
		else
		{
			printf("User receive data: %s\n", readbuf);
		}
		break;
	case 'w':
		if (argc != 4) //进行鲁棒性检查
		{
			printf("Unknow operation, use the formate: ./APPNAME /dev/DRIVENAME w \"USERDATE\" to write date to kernel.\n");
			return -2;
		}
		memcpy(writebuf, argv[3], strlen(argv[3])); //将内容拷贝到缓冲区
		retvalue = write(fd, writebuf, 50);			//写数据
		if (retvalue < 0)
		{
			printf("Write file %s failed!\n", filename);
		}
		else
		{
			printf("Write file success!\n");
		}
		break;
	default:
		printf("Unknow Operation: %d\n", *argv[2]);
		break;
	}

	retvalue = close(fd); //关闭设备
	if (retvalue < 0)
	{
		printf("Can't close file %s\n", filename);
		return -1;
	}

	return 0;
}