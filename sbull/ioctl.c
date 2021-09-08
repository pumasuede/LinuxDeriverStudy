#include<stdlib.h>
#include<stdio.h>
//#include<sys/ioctl.h>
#include<fcntl.h>
#include<errno.h>
#include<sys/types.h>
#include<linux/ioctl.h>
#include<linux/hdreg.h>


#define SCULL_IOC_MAGIC 'k'
#define SCULL_IOCRESET _IO(SCULL_IOC_MAGIC, 0)
#define SCULL_IOCSQSET _IOW(SCULL_IOC_MAGIC, 1, int) 
#define SCULL_IOCTQSET _IO(SCULL_IOC_MAGIC, 2)
#define SCULL_IOCGQSET _IOR(SCULL_IOC_MAGIC, 3, int) 
#define SCULL_IOCQQSET _IO(SCULL_IOC_MAGIC, 4)

//extern int errno;

int main(void){
      int sbulla;
      int qsetq, qsets;
      int ret;
      int err;
      struct hd_geometry  geo;
      sbulla = open("/dev/sbulla", O_RDWR);
      printf("HDIO_GETGEO:%x\n", HDIO_GETGEO);
      ret = ioctl(sbulla, HDIO_GETGEO, &geo);
      //ret = ioctl(sbulla, 0x301, &geo);
      if (ret == -1) {
          err = errno;
        printf("get geo occur errno:%d, %s\n", errno, strerror(err));
      }
      printf("heads:%d, sectors:%d, cylinders:%d, start:%d\n", geo.heads,
	geo.sectors, geo.cylinders, geo.start);
      close(sbulla);
}

