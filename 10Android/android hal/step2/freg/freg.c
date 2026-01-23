#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/errno.h>

#include "freg.h"

//主从设备号，通过alloc_chrdev_region方法分配
static int freg_major = 0;
static int freg_minor = 0;

static struct fake_reg_dev* freg_dev = NULL;
static struct class* freg_class = NULL;//这是C语言，没有class关键字。这里的class是 设备类（device classes）

struct proc_dir_entry* entry, *entry_dir;

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

/* 设备属性操作方法*/
static ssize_t freg_reg1_show(struct device* dev, struct device_attribute* attr, char* buf);
static ssize_t freg_reg1_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count);

//DEVICE_ATTR(_name, _mode, _show, _store)
//扩展成struct device_attribute dev_attr_reg1 = __ATTR(reg1, (00400|00040|00004) | 00200, freg_reg1_show, freg_reg1_store)
static DEVICE_ATTR(reg1, S_IRUGO | S_IWUSR, freg_reg1_show, freg_reg1_store);

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
    if(*f_pos > 0){
        ret = 0;
    }else{
        *buf = dev->reg1;
        *f_pos = 1;
        ret = 1;
    }
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
    printk(KERN_ALERT"freg_write count=%d, offset=%d\n", count, *f_pos);

    dev->reg1 = *buf;
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
    return 0;
}

static ssize_t __freg_get_reg1(struct fake_reg_dev* dev, char* buf) {
    int val = 0;
    if(down_interruptible(&(dev->sem))){
        return -ERESTARTSYS;
    }
    val = dev->reg1;
    up(&(dev->sem));
    return snprintf(buf, PAGE_SIZE, "%d\n", val);
}
static ssize_t __freg_set_reg1(struct fake_reg_dev* dev, const char* buf, size_t count) {
    int val = 0;

    val = simple_strtol(buf, NULL, 10);
    if(down_interruptible(&(dev->sem))){
        return -ERESTARTSYS;
    }
    dev->reg1 = (char)val;
    up(&(dev->sem));
    return count;
}

static ssize_t freg_reg1_show(struct device* dev, struct device_attribute* attr, char* buf){
    /*device结构体的数据结构：
    struct device {
     struct device *parent;
     struct device_private *p;   //负责保存driver核心部分的数据
     struct kobject kobj;
     const char *init_name;
     ....
     struct device_driver *driver;
      .......
     struct device_node *of_node;   //负责保存device_tree中相应的node地址
     .......
     const struct attribute_group **groups;
     ......

     函数dev_get_drvdata主要是返回device->p->driver_data指针。
    */
    struct fake_reg_dev* hdev = (struct fake_reg_dev*) dev_get_drvdata(dev);
    return __freg_get_reg1(hdev, buf);
}
static ssize_t freg_reg1_store(struct device* dev, struct device_attribute* attr, const char* buf, size_t count){
    struct fake_reg_dev* hdev = (struct fake_reg_dev*)dev_get_drvdata(dev);
    return __freg_set_reg1(hdev, buf, count);
}

static ssize_t freg_proc_read(struct file* filp, char* page, size_t count, loff_t* off){
    printk(KERN_ALERT"freg_proc_read count = %d, off=%d\n", count, *off);
    if(*off > 0){
        return 0;
    }
    if(down_interruptible(&(freg_dev->sem))){
        return -ERESTARTSYS;
    }
    *page = freg_dev->reg1;
    *off += 1;
    up(&(freg_dev->sem));
    return 1;
}
static ssize_t freg_proc_write(struct file* filp, const char __user *buf, size_t len, loff_t* off){
    ssize_t err = 0;
    printk(KERN_ALERT"freg_write 1\n");
    if(down_interruptible(&(freg_dev->sem))){
        printk(KERN_ALERT"freg_write interrupted\n");
        return -ERESTARTSYS;
    }
    printk(KERN_ALERT"freg_write len=%d, offset=%d\n", len, *off);

    freg_dev->reg1 = *buf;
    err =  len;//返回count。如果返回0或一个小于count的数，上层调用方会从返回的位置继续写。
out:
    up(&(freg_dev->sem));
    return err;
}

struct file_operations proc_fops = {
        .read = freg_proc_read,
        .write = freg_proc_write,
        .owner = THIS_MODULE
    };
/* 创建/proc/freg_dir/freg文件 */
static void freg_create_proc(void){
    entry_dir = proc_mkdir("freg_dir", NULL);//parent为NULL表示父目录为/proc
    if(!entry_dir){
        printk(KERN_DEBUG "could not create freg_dir in procfs");
        return;
    }
    entry = proc_create(FREG_DEVICE_PROC_NAME, 0, entry_dir, &proc_fops);
    if(!entry){
        printk(KERN_DEBUG "Could not create /proc/freg_dir/freg");
    }
}

/* 删除/proc/freg_dir和里面的文件 */
static void freg_remove_proc(void){
    remove_proc_entry(FREG_DEVICE_PROC_NAME, entry_dir);
    proc_remove(entry_dir);
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

    //在/sys/class/目录下创建freg目录
    freg_class = class_create(THIS_MODULE, FREG_DEVICE_CLASS_NAME);
    if(IS_ERR(freg_class)){//class_create这种函数，返回值并不是只判断是否为NULL这么简单，应该用IS_ERR。
        err = PTR_ERR(freg_class);
        printk(KERN_ALERT"failed to create freg device class.\n");
        goto destory_cdev;
    }
    
    //创建freg设备。/sys/class/freg/目录下会创建一系列文件。/dev/目录会出现freg设备文件。
    struct device* temp = NULL;
    temp = device_create(freg_class, NULL, dev, NULL, "%s", FREG_DEVICE_FILE_NAME);
    if(IS_ERR(freg_class)){//返回值不能简单判断是否为NULL
        err = PTR_ERR(freg_class);
        printk(KERN_ALERT"failed to create freg device.\n");
        goto destory_class;
    }

    //在/sys/class/freg/freg目录创建属性文件reg1
    //对attribute文件的读写操作会由VFS转到结构体dev_attr_reg1里面的freg_reg1_show和freg_reg1_store
    err = device_create_file(temp, &dev_attr_reg1);
    if(err < 0){
        printk(KERN_ALERT"failed to create attribute reg1 of freg device.\n");
        goto destory_device;
    }
    dev_set_drvdata(temp, freg_dev);//对应freg_reg1_store和freg_reg1_show里的dev_get_drvdata


    /*创建/proc/freg */
    freg_create_proc();

    printk(KERN_ALERT"Succeeded to initialize freg device.\n");
    return 0;

destory_device:
    device_destroy(freg_class, dev);
destory_class:
    class_destroy(freg_class);
destory_cdev:
    cdev_del(&(freg_dev->dev));
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

    /* 删除/proc/freg */
    freg_remove_proc();

    if(freg_class){//销毁设备和设备类别
        device_destroy(freg_class, MKDEV(freg_major, freg_minor));
        class_destroy(freg_class);
    }

    //删除字符设备，释放内存
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