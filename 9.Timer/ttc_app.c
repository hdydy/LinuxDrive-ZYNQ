#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd, ret = 0;
    char *filename;
    char writebuf[10] = {0};

    filename = argv[1];

    fd = open(filename, O_RDWR); //打开设备
    if (fd < 0)
    {
        printf("Can't open file %s\n", filename);
        return -1;
    }

    memcpy(writebuf, argv[2], strlen(argv[2])); //将内容拷贝到缓冲区
    ret = write(fd, writebuf, strlen(argv[2])); //写数据
    if (ret < 0)
    {
        printf("Write file %s failed!\n", filename);
    }
    else
    {
        printf("Write file success!\n");
    }

    ret = close(fd); //关闭设备
    if (ret < 0)
    {
        printf("Can't close file %s\n", filename);
        return -1;
    }

    return 0;
}