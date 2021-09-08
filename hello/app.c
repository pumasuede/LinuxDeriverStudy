#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "misc_data.h"

int main(void)
{
    int fd = open("/dev/xy_misc", O_RDWR);
    if (fd < 0)
    {
        printf("open fail:%s\n", strerror(errno));
        return -1;
    }

    int ret = 0;
    struct miscdata data = {
        .val = 18,
        .str = "xy_misc: hello",
        .size = sizeof("xy_misc: hello"),
    };

    if ((ret = ioctl(fd, XY_MISC_IOC_SET, &data)) < 0)
    {
        printf("ioctl set fail:%s\n", strerror(errno));
    }

    struct miscdata getdata;
    if ((ret = ioctl(fd, XY_MISC_IOC_GET, &getdata)) < 0)
    {
        printf("ioctl get fail:%s\n", strerror(errno));
    }

    printf("get val:%d, str:%s, size: %d\n", getdata.val, getdata.str, getdata.size);

    if ((ret = ioctl(fd, XY_MISC_IOC_PRINT, NULL)) < 0)
    {
        printf("ioctl print fail:%s\n", strerror(errno));
    }

    close(fd);
    return ret;
}
