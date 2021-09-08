#ifndef XY_MISC_H_
#define XY_MISC_H_
#include <linux/ioctl.h>

#define STR_LEN 32
struct miscdata {
    int val;
    char str[STR_LEN];
    unsigned int size;
};

#define XY_MISC_IOC_MAGIC 'x'
#define XY_MISC_IOC_PRINT _IO(XY_MISC_IOC_MAGIC, 1)
#define XY_MISC_IOC_GET _IOR(XY_MISC_IOC_MAGIC, 2, struct miscdata)
#define XY_MISC_IOC_SET _IOW(XY_MISC_IOC_MAGIC, 3, struct miscdata)
#define XY_MISC_IOC_MAXNR 3
#endif
