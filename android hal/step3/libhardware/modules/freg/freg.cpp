#define LOG_TAG "FregHALStub"

#include <hardware/freg.h>


#include <fcntl.h>
#include <errno.h>
#include <malloc.h>

#include <cstring>


#include <cutils/log.h>
#include <cutils/atomic.h>

#define DEVICE_NAME "/dev/freg"
#define MODULE_NAME "Freg"
#define MODULE_AUTHOR "575629873@qq.com"

/*设备打开和关闭*/
static int freg_device_open(const struct hw_module_t* module, const char* id, struct hw_device_t** device);
static int freg_device_close(struct hw_device_t* device);

/*寄存器读写*/
static int freg_get_val(struct freg_device_t* dev,int* val);
static int freg_set_val(struct freg_device_t* dev,int val);

static struct hw_module_methods_t freg_module_methods = {
    .open = freg_device_open
};

//module名必须是HAL_MODULE_INFO_SYM。因为它是规定的入口
//#define HAL_MODULE_INFO_SYM         HMI
//hardware.c里面用dlsym(so_handle, "HMI")函数得到hw_module_t结构体的指针
struct freg_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,//tag必须是HARDWARE_MODULE_TAG，标志这是一个硬件抽象层模块结构体
        .version_major = 1,
        .version_minor = 0,
        .id = FREG_HARDWARE_MODULE_ID,
        .name = MODULE_NAME,
        .author = MODULE_AUTHOR,
        .methods = &freg_module_methods
    }
};

static int freg_device_open(const struct hw_module_t* module, const char* id, struct hw_device_t** device){
    if(!strcmp(id, FREG_HARDWARE_DEVICE_ID)) {
        struct freg_device_t* dev;
        dev = (struct freg_device_t*)malloc(sizeof(struct freg_device_t));
        if(!dev) {
            ALOGE("Failed to alloc space for freg_device_t.");
            return -EFAULT;
        }
        memset(dev, 0, sizeof(struct freg_device_t));

        dev->common.tag = HARDWARE_MODULE_TAG;
        dev->common.version = 0;
        dev->common.module = (hw_module_t*)module;
        dev->common.close = freg_device_close;
        dev->set_val = freg_set_val;
        dev->get_val = freg_get_val;

        if((dev->fd = open(DEVICE_NAME, O_RDWR)) == -1){
            ALOGE("Failed to open device file /dev/freg -- %s", strerror(errno));
            free(dev);
            return -EFAULT;
        }
        *device = &(dev->common);
        ALOGI("open device file /dev/freg successfully");

        return 0;
    }
    return -EFAULT;
}
static int freg_device_close(struct hw_device_t* device){
    struct freg_device_t* freg_device = (struct freg_device_t*)device;
    if(freg_device) {
        close(freg_device->fd);
        free(freg_device);
    }
    return 0;
}

static int freg_get_val(struct freg_device_t* dev,int* val){
    if(!dev){
        ALOGE("Null dev pointer");
        return -EFAULT;
    }
    if(!val) {
        ALOGE("Null val pointer");
        return -EFAULT;
    }
    lseek(dev->fd, 0, SEEK_SET);
    read(dev->fd, val, sizeof(*val));
    ALOGI("Get value %d from device file /dev/freg", *val);
    return 0;
}
static int freg_set_val(struct freg_device_t* dev,int val){
    if(!dev){
        ALOGE("Null dev pointer");
        return -EFAULT;
    }
    ALOGI("Set value %d to device file /dev/freg", val);
    write(dev->fd, &val, sizeof(val));
    return 0;
}