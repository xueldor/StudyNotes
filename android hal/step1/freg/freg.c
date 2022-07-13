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

//�����豸�ţ�ͨ��alloc_chrdev_region��������
static int freg_major = 0;
static int freg_minor = 0;

static struct fake_reg_dev* freg_dev = NULL;


//����
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

//ʵ��
static int freg_open(struct inode* inode, struct file* filp) {
    struct fake_reg_dev* dev;
    /*
    #define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})
    �ں��е�container_of�������ǣ�
    1. type�����ͣ����硰struct fake_reg_dev��
    2. member��type�ṹ������һ����Ա��member(define�滻�ɴ������ģ����������dev)��ע�������dev����������ġ�struct fake_reg_dev* dev��������fake_reg_dev������"struct cdev dev;"��dev��
    3. ptr��member�ĵ�ַ���������fake_reg_dev�е�dev�Ķ����ַ
    4. offsetof�ǣ����member��type�ṹ���е�ƫ������
    5. container_of�����þ��ǣ��Ѿ����˽ṹ���е�һ�������õ�����ṹ�塣
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
    //����ͬ������ȡ�ź�����
    //down_interruptible���ڻ�ȡ�����ź�����ʱ�򣬽���"���ж�"��˯�ߡ�"���ж�"��ʾ����Ctrl+c�˳�
    if(down_interruptible(&(dev->sem))){
        printk(KERN_ALERT"freg_read interrupted\n");
        return -ERESTARTSYS;
    }
    printk(KERN_ALERT"freg_read 2,f_pos=%d ,count=%d\n", *f_pos, count);
    loff_t offset = *f_pos < 4 ? *f_pos : 4;
    if(offset + count > 4){
        count = 4 - offset;
    }
    if(copy_to_user(buf, (char*)&(dev->reg1) + offset, count)){//��reg��ֵ���Ƶ��û������������ں��������read���������buf��
        ret = -EFAULT;
        printk(KERN_ALERT"freg_read efault\n");
        goto out;
    }
    printk(KERN_ALERT"freg_read 3 ret=%d\n", count);
    *f_pos += count;
    ret =  count;
out:
    //�ͷ��ź�������down_interruptible�ɶ�
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
    if(copy_from_user((char*)dev + *f_pos, buf, num)){//�����ݴ�buf������reg
        err = -EFAULT;
        goto out;
    }
    *f_pos += count;
    err =  count;//����count���������0��һ��С��count�������ϲ���÷���ӷ��ص�λ�ü���д��
out:
    up(&(dev->sem));
    return err;
}

/*��ʼ���豸*/
static int __freg_setup_dev(struct fake_reg_dev* dev){
    int err;
    dev_t devno = MKDEV(freg_major, freg_minor);

    memset(dev, 0, sizeof(struct fake_reg_dev));

    /*��ʼ���ַ��豸*/
    cdev_init(&(dev->dev), &freg_fops);
    dev->dev.owner = THIS_MODULE;
    dev->dev.ops = &freg_fops;

    /*ע���ַ��豸*/
    err = cdev_add(&(dev->dev), devno, 1);
    if(err){
        return err;
    }
    /*��ʼ���ź���,��ֵΪ1����ֻ����һ��ӵ����*/
    sema_init(&(dev->sem), 1);//init_MUTEX�Ѿ��ϳ�, �°汾ʹ��sema_init 
    dev->reg1 = 0;//��ʼ���Ĵ���
    dev->reg2 = 0;
    dev->reg3 = 0;
    dev->reg4 = 0;
    return 0;
}

static int __init freg_init(void){
    int err = -1;
    dev_t dev = 0;
    printk(KERN_ALERT"Initializing freg device.\n");

    //��̬�������豸�źʹ��豸�ţ�д��dev_t dev��
    err = alloc_chrdev_region(&dev, 0, 1, FREG_DEVICE_NODE_NAME);
    if(err < 0){
        printk(KERN_ALERT"Failed to alloc char dev region.\n");
        goto fail;
    }
    freg_major = MAJOR(dev);//��dev�õ����豸��
    freg_minor = MINOR(dev);//��dev�õ����豸��

    //�ں��ڴ��з���freg�豸�ṹ��
    freg_dev = kmalloc(sizeof(struct fake_reg_dev), GFP_KERNEL);
    if(!freg_dev){
        err = -ENOMEM;
        printk(KERN_ALERT"Failed to alloc freg device.\n");
        goto unregister;
    }
    //��ʼ���豸
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
        kfree(freg_dev);//�ͷ��ڴ�
    }
    unregister_chrdev_region(devno, 1);//�ͷ��豸��
}
MODULE_AUTHOR("xue@163.com");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A Fake Register Driver!");
MODULE_ALIAS("Fake Register!");

module_init(freg_init);
module_exit(freg_exit);