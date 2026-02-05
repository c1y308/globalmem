#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/poll.h>


#define GLOBALFIFO_SIZE 0x1000
#define MEM_CLEAR 0x1
#define GLOBALFIFO_MAJOR 230  // 如果为0，动态分配主设备号
#define DEVICE_NUM 10 // 设备数量


struct globalfifo_dev
{
    struct cdev cdev;
    unsigned int current_len;  // 当前FIFO中有效数据长度
    unsigned char mem[GLOBALFIFO_SIZE];
    struct mutex mutex;
    wait_queue_head_t r_queue;
    wait_queue_head_t w_queue;
};


static int globalfifo_major = GLOBALFIFO_MAJOR;
struct globalfifo_dev *globalfifo_devp;  // 比索引访问更加灵活


static int globalfifo_open(struct inode *inode, struct file *filp)
{
    struct globalfifo_dev *dev = container_of(inode->i_cdev, struct globalfifo_dev, cdev);
    filp->private_data = dev;
    return 0;
}


static int globalfifo_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static long globalfifo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct globalfifo_dev *dev = filp->private_data;
    switch (cmd)
    {
    case MEM_CLEAR:
        mutex_lock(&dev->mutex);
        memset(dev->mem, 0, GLOBALFIFO_SIZE);
        mutex_unlock(&dev->mutex);
        break;
    default:
        return -EINVAL;
    }
    return 0;
}


static ssize_t globalfifo_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
    int ret = 0;
    struct globalfifo_dev *dev = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);

    mutex_lock(&dev->mutex);
    add_wait_queue(&dev->r_queue, &wait);

    while (dev->current_len == 0){  //没有数据可读
        if (filp->f_flags & O_NONBLOCK){  // 非阻塞模式，直接返回
            ret = -EAGAIN;
            goto out;
        }
        __set_current_state(TASK_INTERRUPTIBLE);
        mutex_unlock(&dev->mutex);

        schedule();

        if (signal_pending(current)){
            ret = -ERESTARTSYS;
            goto out2;
        }

        mutex_lock(&dev->mutex);
    }

    if(count > dev->current_len)
        count = dev->current_len;


    if (copy_to_user(buf, dev->mem, count)){
        ret = -EFAULT;
        goto out;
    }
    else{
        memcpy(dev->mem, dev->mem + count, dev->current_len - count);
        dev->current_len -= count;
        printk(KERN_INFO "globalfifo_read: read %zu bytes, current_len: %d\n", count, dev->current_len);
        wake_up_interruptible(&dev->w_queue);
        ret = count;
    }
    out:
    mutex_unlock(&dev->mutex);
    out2:
    remove_wait_queue(&dev->r_queue, &wait);
    set_current_state(TASK_RUNNING);
    return ret;
}


static ssize_t globalfifo_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    int ret = 0;
    struct globalfifo_dev *dev = filp->private_data;
    DECLARE_WAITQUEUE(wait, current);

    mutex_lock(&dev->mutex);
    add_wait_queue(&dev->w_queue, &wait);

    while (dev->current_len == GLOBALFIFO_SIZE){  // 没有空间可写
        if (filp->f_flags & O_NONBLOCK){  // 非阻塞模式，直接返回
            ret = -EAGAIN;
            goto out;
        }
        __set_current_state(TASK_INTERRUPTIBLE);
        mutex_unlock(&dev->mutex);

        schedule();

        if (signal_pending(current)){
            ret = -ERESTARTSYS;
            goto out2;
        }

        mutex_lock(&dev->mutex);
    }

    if(count > GLOBALFIFO_SIZE - dev->current_len)
        count = GLOBALFIFO_SIZE - dev->current_len;

    if (copy_from_user(dev->mem + dev->current_len, buf, count)){
        ret = -EFAULT;
        goto out;
    }
    else{
        dev->current_len += count;
        printk(KERN_INFO "globalfifo_write: write %zu bytes, current_len: %u\n", count, dev->current_len);
        wake_up_interruptible(&dev->r_queue);
        ret = count;
    }
    out:
    mutex_unlock(&dev->mutex);
    out2:
    remove_wait_queue(&dev->w_queue, &wait);
    set_current_state(TASK_RUNNING);
    return ret;
}


static loff_t globalfifo_llseek(struct file *filp, loff_t offset, int whence){
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
        ret = GLOBALFIFO_SIZE - offset;
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


static unsigned int globalfifo_poll(struct file *filp, struct poll_table_struct *wait){
    unsigned int mask = 0;
    struct globalfifo_dev *dev = filp->private_data;  // 获取设备结构体指针
    mutex_lock(&dev->mutex);
    poll_wait(filp, &dev->r_queue, wait);
    poll_wait(filp, &dev->w_queue, wait);
    if (dev->current_len){
        mask |= POLLIN | POLLRDNORM;
    }
    if (dev->current_len < GLOBALFIFO_SIZE){
        mask |= POLLOUT | POLLWRNORM;
    }
    mutex_unlock(&dev->mutex);
    return mask;
}


static const struct file_operations globalfifo_fops =
{
    .owner = THIS_MODULE,
    .read = globalfifo_read,
    .write = globalfifo_write,
    .unlocked_ioctl = globalfifo_ioctl,
    .open = globalfifo_open,
    .release = globalfifo_release,
    .llseek = globalfifo_llseek,
    .poll = globalfifo_poll,
};


/* 初始化cdev */
static void globalfifo_setup_cdev(struct globalfifo_dev *dev, int index)
{
    int err, devno = MKDEV(globalfifo_major, index);
    cdev_init(&dev->cdev, &globalfifo_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
    {
        printk(KERN_ERR "Error %d adding globalfifo %d", err, index);
        return;
    }
}

static __init int globalfifo_init(void)
{
    int ret = 0;
    int i;
    dev_t devno = MKDEV(globalfifo_major, 0);
    if(globalfifo_major){  // 指定了主设备号，申请指定的设备号区间（主设备号和设备数量决定）
        ret = register_chrdev_region(devno, DEVICE_NUM, "globalfifo");
        if (ret < 0)
        {
            printk(KERN_ERR "globalfifo_init: register_chrdev_region failed %d", ret);
            return ret;
        }
    }else{  // 动态分配一个主设备号以及提前申请到设备号区间
        ret = alloc_chrdev_region(&devno, 0, DEVICE_NUM, "globalfifo");
        if (ret < 0)
        {
            printk(KERN_ERR "globalfifo_init: alloc_chrdev_region failed %d", ret);
            return ret;
        }
        globalfifo_major = MAJOR(devno);
    }


    globalfifo_devp = kmalloc(sizeof(struct globalfifo_dev) * DEVICE_NUM, GFP_KERNEL);
    if (!globalfifo_devp)
    {
        ret = -ENOMEM;
        goto fail_malloc;
    }
    memset(globalfifo_devp, 0, sizeof(struct globalfifo_dev) * DEVICE_NUM);
    for (i = 0; i < DEVICE_NUM; i++)
    {
        globalfifo_setup_cdev(globalfifo_devp + i, i);
        mutex_init(&globalfifo_devp[i].mutex);
        init_waitqueue_head(&globalfifo_devp[i].r_queue);
        init_waitqueue_head(&globalfifo_devp[i].w_queue);
    }
    printk(KERN_INFO "globalfifo_init\n");

    return 0;

    fail_malloc:
    unregister_chrdev_region(devno, DEVICE_NUM);
    return ret;
}


static __exit void globalfifo_exit(void)
{
    int i = 0;
    for (i = 0; i < DEVICE_NUM; i++)
    {
        mutex_destroy(&globalfifo_devp[i].mutex);
        cdev_del(&globalfifo_devp[i].cdev);
    }
    kfree(globalfifo_devp);
    unregister_chrdev_region(MKDEV(globalfifo_major, 0), DEVICE_NUM);
    printk(KERN_INFO "globalfifo_exit\n");
}


module_init(globalfifo_init);
module_exit(globalfifo_exit);
MODULE_DESCRIPTION("A simple global memory driver");
MODULE_AUTHOR("C1Y308");
MODULE_LICENSE("GPL V2");
