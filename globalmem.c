#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define GLOBALMEM_SIZE 0x1000
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 230  // 如果为0，动态分配主设备号
#define DEVICE_NUM 10 // 设备数量
struct globalmem_dev
{
    struct cdev cdev;
    unsigned char mem[GLOBALMEM_SIZE];
};


static int globalmem_major = GLOBALMEM_MAJOR;
struct globalmem_dev *globalmem_devp;  // 比索引访问更加灵活


static int globalmem_open(struct inode *inode, struct file *filp)
{
    struct globalmem_dev *dev = container_of(inode->i_cdev, struct globalmem_dev, cdev);
    filp->private_data = dev;
    return 0;
}


static int globalmem_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct globalmem_dev *dev = filp->private_data;
    switch (cmd)
    {
    case MEM_CLEAR:
        memset(dev->mem, 0, GLOBALMEM_SIZE);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}


static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos){
    unsigned long p = *ppos;
    size_t count = size;
    int ret = 0;
    if (p >= GLOBALMEM_SIZE)
        return 0;
    if (p + count > GLOBALMEM_SIZE)
        count = GLOBALMEM_SIZE - p;


    if (copy_to_user(buf, globalmem_devp->mem + p, count))
        return -EFAULT;
    else{
        *ppos += count;
        ret = count;
        printk(KERN_INFO "globalmem_read: read %d bytes from %ld\n", ret, p);
    }

    return ret;
}


static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos){
    unsigned long p = *ppos;
    size_t count = size;
    int ret = 0;
    if (p >= GLOBALMEM_SIZE)
        return 0;
    if (p + count > GLOBALMEM_SIZE)
        count = GLOBALMEM_SIZE - p;


    if (copy_from_user(globalmem_devp->mem + p, buf, count))
        return -EFAULT;
    else{
        *ppos += count;
        ret = count;
        printk(KERN_INFO "globalmem_write: write %d bytes to %ld\n", ret, p);
    }

    return ret;
}


static loff_t globalmem_llseek(struct file *filp, loff_t offset, int whence){
    loff_t ret = 0;
    switch (whence)
    {
    case SEEK_SET:
        ret = offset;
        break;
    case SEEK_CUR:
        ret = filp->f_pos + offset;
        break;
    case SEEK_END:
        ret = GLOBALMEM_SIZE - offset;
        break;
    default:
        ret = -EINVAL;
        break;
    }
    if (ret < 0)
        return ret;
    filp->f_pos = ret;
    return ret;
}

static const struct file_operations globalmem_fops =
{
    .owner = THIS_MODULE,
    .read = globalmem_read,
    .write = globalmem_write,
    .unlocked_ioctl = globalmem_ioctl,
    .open = globalmem_open,
    .release = globalmem_release,
    .llseek = globalmem_llseek,
};


/* 初始化cdev */
static void globalmem_setup_cdev(struct globalmem_dev *dev, int index)
{
    int err, devno = MKDEV(globalmem_major, index);
    cdev_init(&dev->cdev, &globalmem_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding globalmem %d", err, index);
    }
}

static __init int globalmem_init(void)
{
    int ret = 0;
    dev_t devno = MKDEV(globalmem_major, 0);
    if(globalmem_major){  // 指定了主设备号，申请指定的设备号区间
        ret = register_chrdev_region(devno, DEVICE_NUM, "globalmem");
        if (ret < 0)
        {
            printk(KERN_ERR "globalmem_init: register_chrdev_region failed %d", ret);
            return ret;
        }
    }else{  // 动态分配一个主设备号以及申请到设备号区间
        ret = alloc_chrdev_region(&devno, 0, DEVICE_NUM, "globalmem");
        if (ret < 0)
        {
            printk(KERN_ERR "globalmem_init: alloc_chrdev_region failed %d", ret);
            return ret;
        }
        globalmem_major = MAJOR(devno);
    }

    globalmem_devp = kmalloc(sizeof(struct globalmem_dev) * DEVICE_NUM, GFP_KERNEL);
    if (!globalmem_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }
    memset(globalmem_devp, 0, sizeof(struct globalmem_dev) * DEVICE_NUM);
    int i = 0;
    for (i = 0; i < DEVICE_NUM; i++)
    {
        globalmem_setup_cdev(globalmem_devp + i, i);
    }
    printk(KERN_INFO "globalmem_init\n");

    return 0;

    fail_malloc:
    unregister_chrdev_region(devno, 1);
    return ret;
}


static __exit void globalmem_exit(void)
{
    int i = 0;
    for (i = 0; i < DEVICE_NUM; i++)
    {
        cdev_del(&globalmem_devp[i].cdev);
    }
    kfree(globalmem_devp);
    unregister_chrdev_region(MKDEV(globalmem_major, 0), DEVICE_NUM);
    printk(KERN_INFO "globalmem_exit\n");
}


module_init(globalmem_init);
module_exit(globalmem_exit);
MODULE_DESCRIPTION("A simple global memory driver");
MODULE_AUTHOR("C1Y308");
MODULE_LICENSE("GPL V2");
