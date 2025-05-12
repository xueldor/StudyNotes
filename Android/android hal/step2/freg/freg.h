#ifndef _FAKE_REG_H_
#define _FAKE_REG_H_

#include <linux/cdev.h>//字符设备
#include <linux/semaphore.h>//信号量

//定义虚拟设备freg在设备文件系统中名称
#define FREG_DEVICE_NODE_NAME "freg"
#define FREG_DEVICE_FILE_NAME "freg"
#define FREG_DEVICE_PROC_NAME "freg"
#define FREG_DEVICE_CLASS_NAME "freg"

struct fake_reg_dev {
    char reg1;
    
    struct semaphore sem;
    struct cdev dev;
};


#endif