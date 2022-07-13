#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/errno.h>

#include "freg.h"

//主从设备号，通过alloc_chrdev_region方法分配
static int freg_major = 0;
static int freg_minor = 0;

static struct fake_reg_dev* freg_dev = NULL;


//声明
static int freg_open(struct inode* inode, struct file* filp);
static int freg_release(struct inode* inode, struct file* filp);
static ssize_t freg_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos);
static ssize_t freg_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos);

static struct file_operations freg_fops = {
    .owner = THIS_MODULE,
    .open = freg_open,
    .release = freg_release,
    .read = freg_read,
    .write = freg_write
};

//实现
static int freg_open(struct inode* inode, struct file* filp) {
    struct fake_reg_dev* dev;
    /*
    #define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
    内核中的container_of，作用是：
    1. type是类型，比如“struct fake_reg_dev”
    2. member是type结构体里有一个成员叫member(define替换成传进来的，比如下面的dev)。注意这里的dev，不是上面的“struct fake_reg_dev* dev”，而是fake_reg_dev”里面"struct cdev dev;"的dev。
    3. ptr是member的地址。这里就是fake_reg_dev中的dev的对象地址
    4. offsetof是，获得member在type结构体中的偏移量。
    5. container_of的作用就是，已经有了结构体中的一个对象，拿到这个结构体。
    */
    dev = container_of(inode->i_cdev, struct fake_reg_dev, dev);
    filp->private_data = dev;
    return 0;
}
static int freg_release(struct inode* inode, struct file* filp){
    return 0;
}
static ssize_t freg_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos){
    ssize_t ret = 0;
    struct fake_reg_dev* dev = filp->private_data;
    printk(KERN_ALERT"freg_read 1\n");
    //访问同步。获取信号量。
    //down_interruptible，在获取不到信号量的时候，进入"可中断"的睡眠。"可中断"表示可以Ctrl+c退出
    if(down_interruptible(&(dev->sem))){
        printk(KERN_ALERT"freg_read interrupted\n");
        return -ERESTARTSYS;
    }
    printk(KERN_ALERT"freg_read 2,f_pos=%d ,count=%d\n", *f_pos, count);
    loff_t offset = *f_pos < 4 ? *f_pos : 4;
    if(offset + count > 4){
        count = 4 - offset;
    }
    if(copy_to_user(buf, (char*)&(dev->reg1) + offset, count)){//将reg的值复制到用户缓冲区，即内核外面调用read方法传入的buf。
        ret = -EFAULT;
        printk(KERN_ALERT"freg_read efault\n");
        goto out;
    }
    printk(KERN_ALERT"freg_read 3 ret=%d\n", count);
    *f_pos += count;
    ret =  count;
out:
    //释放信号量。和down_interruptible成对
    up(&(dev->sem));
    return ret;
}
static ssize_t freg_write(struct file* filp, const char __user *buf, size_t count, loff_t* f_pos){
    ssize_t err = 0;
    struct fake_reg_dev* dev = filp->private_data;
    printk(KERN_ALERT"freg_write 1\n");
    if(down_interruptible(&(dev->sem))){
        printk(KERN_ALERT"freg_write interrupted\n");
        return -ERESTARTSYS;
    }
    printk(KERN_ALERT"freg_write 2\n");
    int num = count > 4 ? 4 : count;
    if(copy_from_user((char*)dev + *f_pos, buf, num)){//将数据从buf拷贝到reg
        err = -EFAULT;
        goto out;
    }
    *f_pos += count;
    err =  count;//返回count。如果返回0或一个小于count的数，上层调用方会从返回的位置继续写。
out:
    up(&(dev->sem));
    return err;
}

/*初始化设备*/
static int __freg_setup_dev(struct fake_reg_dev* dev){
    int err;
    dev_t devno = MKDEV(freg_major, freg_minor);

    memset(dev, 0, sizeof(struct fake_reg_dev));

    /*初始化字符设备*/
    cdev_init(&(dev->dev), &freg_fops);
    dev->dev.owner = THIS_MODULE;
    dev->dev.ops = &freg_fops;

    /*注册字符设备*/
    err = cdev_add(&(dev->dev), devno, 1);
    if(err){
        return err;
    }
    /*初始化信号量,初值为1，即只允许一个拥有者*/
    sema_init(&(dev->sem), 1);//init_MUTEX已经废除, 新版本使用sema_init 
    dev->reg1 = 0;//初始化寄存器
    dev->reg2 = 0;
    dev->reg3 = 0;
    dev->reg4 = 0;
    return 0;
}

static int __init freg_init(void){
    int err = -1;
    dev_t dev = 0;
    printk(KERN_ALERT"Initializing freg device.\n");

    //动态分配主设备号和从设备号，写入dev_t dev。
    err = alloc_chrdev_region(&dev, 0, 1, FREG_DEVICE_NODE_NAME);
    if(err < 0){
        printk(KERN_ALERT"Failed to alloc char dev region.\n");
        goto fail;
    }
    freg_major = MAJOR(dev);//从dev得到主设备号
    freg_minor = MINOR(dev);//从dev得到从设备号

    //内核内存中分配freg设备结构体
    freg_dev = kmalloc(sizeof(struct fake_reg_dev), GFP_KERNEL);
    if(!freg_dev){
        err = -ENOMEM;
        printk(KERN_ALERT"Failed to alloc freg device.\n");
        goto unregister;
    }
    //初始化设备
    err = __freg_setup_dev(freg_dev);
    if(err){
        printk(KERN_ALERT"Failed to setup freg device:%d\n", err);
        goto cleanup;
    }
    return 0;

cleanup:
    kfree(freg_dev);
unregister:
    unregister_chrdev_region(MKDEV(freg_major, freg_minor), 1);
fail:
    return err;
}

static void __exit freg_exit(void) {
    dev_t devno = MKDEV(freg_major, freg_minor);
    printk(KERN_ALERT"Destory freg device.\n");

    if(freg_dev){
        cdev_del(&(freg_dev->dev));
        kfree(freg_dev);//释放内存
    }
    unregister_chrdev_region(devno, 1);//释放设备号
}
MODULE_AUTHOR("xue@163.com");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A Fake Register Driver!");
MODULE_ALIAS("Fake Register!");

module_init(freg_init);
module_exit(freg_exit);