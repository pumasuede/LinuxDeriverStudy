#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h> 

#include "misc_data.h"

#define NAME_MAX_LENGTH 32

struct my_misc_dev{
    struct miscdevice misc;
    struct miscdata data;
};

struct my_misc_dev *misc_dev_p;

void print_jiff(struct seq_file *m)
{
    struct timeval tv1;
    struct timespec tv2;
    unsigned long j1;
    u64 j2;

    j1 = jiffies;
    j2 = get_jiffies_64();
    do_gettimeofday(&tv1);
    tv2 = current_kernel_time();
    seq_printf(m, "jiffies is %ld\n", j1);
    seq_printf(m, "get_jiffies_64 is %ld\n", (long)j2);
    seq_printf(m, "do_gettimeofday is %ld:%ld\n", tv1.tv_sec, tv1.tv_usec);
    seq_printf(m, "current_kernel_time is %ld:%ld\n", tv2.tv_sec, tv2.tv_nsec);

    j1 = jiffies;
    j2 = get_jiffies_64();
    do_gettimeofday(&tv1);
    tv2 = current_kernel_time();
    seq_printf(m, "jiffies is %ld\n", j1);
    seq_printf(m, "get_jiffies_64 is %ld\n", (long)j2);
    seq_printf(m, "do_gettimeofday is %ld:%ld\n", tv1.tv_sec, tv1.tv_usec);
    seq_printf(m, "current_kernel_time is %ld:%ld\n", tv2.tv_sec, tv2.tv_nsec);
}

void list_ops(struct seq_file *m)
{    
    typedef struct my_list
    {
        struct list_head list;
        char name[NAME_MAX_LENGTH];
    } my_list;

    int i = 0;
    my_list *node;
    struct list_head head, *p, *n;
    char* names[] = {"nvidia", "amd", "intel", "arm"}; 

    INIT_LIST_HEAD(&head);
    // LIST_HEAD(head);
    for (i = 0; i < sizeof(names)/sizeof(names[0]); i++)
    {
        node = kmalloc(sizeof(my_list), GFP_KERNEL);
        strcpy(node->name, names[i]);
        list_add_tail(&node->list, &head);
    }

    i = 0;
    list_for_each(p, &head)
    {
        node = list_entry(p, my_list, list);
        seq_printf(m, "name of node %d: %s\n", i++, node->name);
    }

    // a more clean mway to travesal list
    i = 0;
    list_for_each_entry(node, &head, list)
    {
        seq_printf(m, "name of my_list[%d]: %s\n", i++, node->name);
    }

    list_for_each_safe(p, n, &head)
    {
        node = list_entry(p, my_list, list);
        seq_printf(m, "deleting %s\n", node->name);
        list_del(p);
        kfree(node);

    }

    i = 0;
    list_for_each_entry(node, &head, list)
    {
        seq_printf(m, "name of node %d: %s\n", i++, node->name);
    }
}

void demo(struct seq_file *m)
{
    void *p;
    char *pe;
    int x = 0x01020304;
    

    seq_printf(m, "VMALLOC_START is %p\n", (void*)VMALLOC_START);
    p = vmalloc(4*128*1204);
    seq_printf(m, "vmalloc: p is %p\n", p);


    p = kmalloc(32, GFP_KERNEL);
    seq_printf(m, "kmalloc: p is %p\n", p);

    seq_printf(m, "HZ is %d\n", HZ);
    seq_printf(m, "page size is 0x%lx\n", PAGE_SIZE);

#ifdef __BIG_ENDIAN
    pe = "big";
#else
    pe = "little";
#endif
    seq_printf(m, "endian: %s\n", pe);

    x = cpu_to_be32(x);
    seq_printf(m, "x: 0x%x\n", x);

}

static int hello_proc_show(struct seq_file *m, void *v) 
{
    //print_jiff(m);
    //demo(m);
    list_ops(m);
    return 0;
}

static int hello_proc_open(struct inode *inode, struct  file *file) 
{
    return single_open(file, hello_proc_show, NULL);
}


int misc_open(struct inode *inode, struct file *filep)
{
    filep->private_data = misc_dev_p;
    return 0;
}

long misc_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct my_misc_dev *devp = (struct my_misc_dev *)(filep->private_data);

    if (_IOC_TYPE(cmd) != XY_MISC_IOC_MAGIC)
        return -EINVAL;

    if (_IOC_NR(cmd) > XY_MISC_IOC_MAXNR)
        return -EINVAL;

    switch(cmd)
    {
        case XY_MISC_IOC_PRINT:
            printk("XY_MISC_IOC_PRINT\n");
            printk("val:%d, size: %d, str: %s\n", devp->data.val, devp->data.size, devp->data.str);
            break;
        case XY_MISC_IOC_SET:
            printk("XY_MISC_IOC_SET\n");
            ret = copy_from_user((unsigned char*)&(devp->data), (unsigned char *)arg, sizeof(struct miscdata));
            printk("set val:%d, size: %d, str: %s\n", devp->data.val, devp->data.size, devp->data.str);
            break;
        case XY_MISC_IOC_GET:
            printk("XY_MISC_IOC_GET\n");
            ret = copy_to_user((unsigned char*)arg,(unsigned char*)&(devp->data), sizeof(struct miscdata));
            break;
        default:
            return -EINVAL;
    }

    return ret;
}

static const struct file_operations hello_proc_fops = {
    .owner = THIS_MODULE,
    .open = hello_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};

static const struct file_operations misc_fops ={
    .owner = THIS_MODULE,
    .open = misc_open,
    .unlocked_ioctl = misc_ioctl,
};

static struct miscdevice xy_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "xy_misc",
    .fops = &misc_fops,
};

static int __init hello_init(void)
{    
    printk(KERN_ALERT "Hello world, raxiao\n");    

    proc_create("raxiao_proc", 0, NULL, &hello_proc_fops);

    misc_dev_p = kmalloc(sizeof(struct my_misc_dev), GFP_KERNEL);
    if (!misc_dev_p)
        return -ENOMEM;

    memset(&(misc_dev_p->data), 0, sizeof(misc_dev_p->data));
    misc_dev_p->misc = xy_misc;

    return misc_register(&(misc_dev_p->misc));
}

static void __exit hello_exit(void)
{    
    printk(KERN_ALERT "Goodbye cruel world, raxiao\n");
    remove_proc_entry("raxiao_proc", NULL);
    misc_deregister(&(misc_dev_p->misc));
    kfree(misc_dev_p);
}

MODULE_LICENSE("GPL");
module_init(hello_init);
module_exit(hello_exit);
