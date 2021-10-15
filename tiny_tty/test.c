#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>

#define max_buffer_size 100 /*recv buffer size*/


int open_serial(int k)
{
    char pathname[20] = {0};
    int ret;

    sprintf(pathname, "/dev/ttty%d", k);
    ret = open(pathname, O_RDWR|O_NOCTTY);
    if(ret == -1)
    {
        perror("open error");
        exit(-1);
    }
    else
        printf("Open %s success\n", pathname);

    return ret;
}

int main()
{
    int fd;
    ssize_t n;
    char recv[max_buffer_size] = {0};
    struct termios opt;

    fd = open_serial(0); /*open device 0*/

    tcgetattr(fd, &opt);
    cfmakeraw(&opt);
    tcsetattr(fd, TCSANOW, &opt);


    printf("ready for receiving data...\n");
    n = read(fd, recv, sizeof(recv));
    if(n == -1)
    {
        perror("read error");
        exit(-1);
    }

    printf("The data received is %s", recv);
    if(close(fd) == -1)
        perror("close error");

    return 0;
}


