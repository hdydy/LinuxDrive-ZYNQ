#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>

int main(int argc, char *argv[])
{
    int fd, ret;
    struct input_event BUT_event;

    //打开设备
    fd = open(argv[1], O_RDWR);
    if (fd < 0)
    {
        printf("Open failed!\n");
        return -1;
    }

    while (1)
    {
        ret = read(fd, &BUT_event, sizeof(struct input_event));
        if (ret)
        {
            switch (BUT_event.type)
            {
            case EV_KEY:
                //按键事件
                if (BUT_event.code == KEY_0) //识别事件的code
                {
                    if (BUT_event.value) //获得事件的传输值
                        printf("BUT1 Press.\n");
                    else
                        printf("BUT1 Release.\n");
                }
                break;
            case EV_SYN:
                //同步事件
                break;
            default:
                printf("Unknow message type.%d\n", BUT_event.type);
                break;
            }
        }
        else
        {
            printf("Read failed!\n");
            close(fd);
            return 0;
        }
    }

    close(fd);
    return 0;
}