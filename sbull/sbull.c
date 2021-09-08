#include<linux/init.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/kernel.h> //printk
#include<linux/genhd.h>
#include<linux/timer.h>
#include<linux/slab.h>
#include<linux/vmalloc.h>
#include<linux/errno.h>
#include<linux/timer.h>
#include <linux/hdreg.h>    /* HDIO_GETGEO */
#include<linux/blkdev.h>
#include "sbull.h"

#define blk_fs_request(rq)      ((rq)->cmd_type == REQ_TYPE_FS)

MODULE_LICENSE("Dual BSD/GPL");

#define KERNEL_SECTOR_SIZE 512
#define INVALIDATE_DELAY 30*HZ


static int sbull_major = 0;
static struct sbull_dev *dev = NULL;
static int nsectors = 11 * 1024 * 2 + 1;
static int hardsect_size = 1024;

int which = 0;
int SBULL_MINORS = 4;

static int sbull_media_changed(struct gendisk *disk) {
    struct sbull_dev* dev = disk->private_data;
    return dev->media_change;
}

static int sbull_revalidate(struct gendisk *gd) {
    struct sbull_dev *dev = gd->private_data;
    if (dev->media_change) {
        dev->media_change = 0;
        memset(dev->data, 0, dev->size * hardsect_size);
    }
    return 0;
}


static int sbull_open(struct block_device *bd,  fmode_t mode) {
    struct sbull_dev *dev = bd->bd_disk->private_data;
    //del_timer_sync(&dev->timer);
    //filp->private_data = dev;
    spin_lock(&dev->lock);
    if (! dev->users) {
        check_disk_change(bd);
    }
    dev->users++;
    spin_unlock(&dev->lock);
    return 0;
}

static void sbull_release(struct gendisk *disk, fmode_t mode) {
    struct sbull_dev *dev = disk->private_data;
    spin_lock(&dev->lock);
    dev->users--;
    /*
    if (! dev->users) {
        dev->timer.expires = jiffies + INVALIDATE_DELAY;
        add_timer(&dev->timer);
    }
    */
    spin_unlock(&dev->lock);
    return;
}

/*
int sbull_ioctl (struct block_device* bd, fmode_t mode,
        unsigned int cmd, unsigned long arg)
{
    long size;
    struct hd_geometry geo;
    struct sbull_dev *dev = bd->bd_disk->private_data;
    printk(KERN_NOTICE "****get the disk geometry,cmd:%x, HDIO_GETGEO:%x\n", cmd, HDIO_GETGEO);
    switch(cmd) {
        case HDIO_GETGEO:
            printk(KERN_NOTICE "get the disk geometry\n");
            size = dev->size*(hardsect_size/KERNEL_SECTOR_SIZE);
            geo.cylinders = (size & ~0x3f) >> 6;
            geo.heads = 4;
            geo.sectors = 16;
            geo.start = 4;
            if (copy_to_user((void __user *) arg, &geo, sizeof(geo)))
                return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}*/

int sbull_ioctl (struct block_device* bd, fmode_t mode,
        unsigned int cmd, unsigned long arg)
{
	return 0;
}

static int sbull_getgeo(struct block_device *bdev, struct hd_geometry *geo) 
{
    unsigned long size;  
    struct sbull_dev *pdev = bdev->bd_disk->private_data;  
  
    size = pdev->size;  
    geo->cylinders = (size & ~0x3f) >> 6;  
    geo->heads   = 4;  
    geo->sectors = 16;  
    geo->start = 0;  
    return 0;
}

static struct block_device_operations sbull_ops = {
    .owner = THIS_MODULE,
    .open = sbull_open,
    .ioctl = sbull_ioctl,
    .release = sbull_release,
    .media_changed = sbull_media_changed,
    .revalidate_disk = sbull_revalidate,
    .getgeo = sbull_getgeo,
};

static void sbull_transfer(struct sbull_dev* dev, unsigned long sector,
                           unsigned long nsect, char* buffer, int write) 
{
    unsigned long offset = sector*KERNEL_SECTOR_SIZE;
    unsigned long nbytes = nsect*KERNEL_SECTOR_SIZE;

    if ((offset + nbytes) > (dev->size*hardsect_size) ) {
        printk(KERN_ERR "beyond-end IO (%ld %ld)", offset, nbytes);
        return;
    }

    if (write) {
        printk(KERN_NOTICE "write data size:%ld,buf:%c, buffer:%s", nbytes, *buffer,buffer);
        memcpy(dev->data + offset, buffer, nbytes);
    }
    else {
        printk(KERN_NOTICE "read data size:%ld", nbytes); 
	    memcpy(buffer, dev->data + offset, nbytes);
    }
}

/*
static void sbull_request(struct request_queue *q) {
   struct request *req;
   while((req = blk_fetch_request(q)) != NULL) {
       struct sbull_dev* dev = req->rq_disk->private_data;
       
       if (! blk_fs_request(req)) {
           printk (KERN_WARNING "skip non-fs request\n");
           __blk_end_request_all(req, 1);
           continue;
       }
       printk(KERN_NOTICE "Req dir %llu, sec %ld, nr %d f %llx\n",
               rq_data_dir(req), (long)blk_rq_pos(req), blk_rq_cur_sectors(req),
               req->cmd_flags);
       sbull_transfer(dev, blk_rq_pos(req), blk_rq_cur_sectors(req),
               req->buffer, rq_data_dir(req));
       __blk_end_request_all(req, 0);
   }
}
*/

static int sbull_xfer_bio(struct sbull_dev *dev, struct bio* bio)
{
    int i;
    struct bio_vec *bvec;

    sector_t sector = bio->bi_sector;
    printk(KERN_NOTICE "bio segemnts is %d", bio->bi_vcnt );

    bio_for_each_segment(bvec, bio, i) {
        char *buffer = __bio_kmap_atomic(bio, i, KM_USER0);
        sbull_transfer(dev, sector, bio_cur_bytes(bio)/KERNEL_SECTOR_SIZE,
                buffer, bio_data_dir(bio)==WRITE);
        sector += bio_cur_bytes(bio)/KERNEL_SECTOR_SIZE;
        __bio_kunmap_atomic(buffer, KM_USER0);
        bio->bi_idx = i;
    }
    return sector - bio->bi_sector;
}

static int sbull_xfer_request(struct sbull_dev *dev,struct request *req)
{
    struct bio *bio;
    int nsect = 0;
    __rq_for_each_bio(bio, req) { 
        nsect += sbull_xfer_bio(dev, bio);
    }
    return nsect;
}

static void sbull_full_request(struct request_queue *q)
{
    int nsects;

    struct sbull_dev *dev = q->queuedata;
    struct request *req = blk_fetch_request(q);
    while (req != NULL) {
        if (req->cmd_type != REQ_TYPE_FS) {
            printk(KERN_NOTICE "Skip non-fs request\n");
            __blk_end_request_all(req,-EIO);
            continue;
        }

        nsects = sbull_xfer_request(dev, req);
        printk(KERN_NOTICE "begin sector:%ld, IO sectors:%d\n", blk_rq_pos(req), nsects);
        /*
        if(!__blk_end_request_cur(req,0)){
            req = NULL;
        }*/
        __blk_end_request(req, 0, (nsects<<9));
        //req = NULL;
	    req = blk_fetch_request(q);
    }
}

void sbull_invalidate(unsigned long ldev) {
    struct sbull_dev* dev = (struct sbull_dev*)ldev;
    spin_lock(&dev->lock);
    if (dev->users||!dev->data)
        printk (KERN_WARNING "sbull: timer check failed\n");
    else
        dev->media_change = 1;
    spin_unlock(&dev->lock);
}

static int sbull_init(void){
    struct gendisk *gd;
    int result;

    sbull_major = register_blkdev(sbull_major, "sbull");
    if (sbull_major <= 0) {
        printk(KERN_WARNING "sbull: unable to get major number\n");
        result =  -EBUSY;
    }

    printk(KERN_NOTICE "sbull_major: %d\n", sbull_major);

    dev = kmalloc(sizeof(struct sbull_dev), GFP_KERNEL);
    if (!dev){
        result = -ENOMEM;
        goto fail;
    }

    memset(dev, 0, sizeof(struct sbull_dev));
    printk(KERN_NOTICE "malloc sbull_dev\n");
    
    dev->size  = nsectors;
    dev->data = vmalloc(dev->size * hardsect_size);
    if (dev->data == NULL) {
        printk(KERN_NOTICE "vmalloc failed\n");
        result = -ENOMEM;
        goto fail;
    }

    printk(KERN_NOTICE "malloc data\n");
    
    spin_lock_init(&dev->lock);
    
    // Init the  timer
    init_timer(&dev->timer);
    dev->timer.data = (unsigned long)dev;
    dev->timer.function = sbull_invalidate;

    // Init the request queue
    //dev->queue = blk_init_queue(sbull_request, &dev->lock);
    dev->queue = blk_init_queue(sbull_full_request, &dev->lock);
    dev->queue->queuedata = dev;
    //blk_queue_hardsect_size(dev->queue, hardsect_size);
    blk_queue_logical_block_size(dev->queue, hardsect_size);

    // Allocate the gen_disk
    gd = alloc_disk(SBULL_MINORS);
    if (gd == NULL) {
        printk(KERN_NOTICE "alloc_disk failure\n");
        result = -ENOMEM;
        goto fail;
    }
    dev->gd = gd;

    gd->major = sbull_major;
    gd->first_minor = which*SBULL_MINORS;
    gd->fops = &sbull_ops;
    gd->private_data = dev;
    gd->queue = dev->queue;
    set_capacity(gd, nsectors*(hardsect_size/KERNEL_SECTOR_SIZE));
    snprintf(gd->disk_name, 32, "sbull%c", which + 'a');
    
    // Make the disk available to the system
    add_disk(gd);
    return 0;

fail:
    unregister_blkdev(sbull_major, "sbull"); 
    return result;
}

static void sbull_exit(void) {
    del_timer_sync(&dev->timer);
    if (dev->gd) {
        del_gendisk(dev->gd);
    }
    if (dev->queue) {
        blk_cleanup_queue(dev->queue);
    }
    if (dev->data){
        vfree(dev->data);
    }
    unregister_blkdev(sbull_major, "sbull");
    kfree(dev);
}

module_init(sbull_init);
module_exit(sbull_exit);
