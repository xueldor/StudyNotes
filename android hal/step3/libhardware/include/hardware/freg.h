#ifndef ANDROID_FREG_INTETFACE_H
#define ANDROID_FREG_INTETFACE_H

#include <hardware/hardware.h>

__BEGIN_DECLS

/*定义模块ID*/
#define FREG_HARDWARE_MODULE_ID "freg"
/*定义设备ID*/
#define FREG_HARDWARE_DEVICE_ID "freg"

/*自定义模块结构体*/
struct freg_module_t {
    struct hw_module_t common;//hw_module_t必须是第一个成员变量
    
};
/*自定义设备结构体*/
struct freg_device_t {
    struct hw_device_t common;
    int fd;
    int (*set_val)(struct freg_device_t* dev,int val);
    int (*get_val)(struct freg_device_t* dev,int* val);
};


__END_DECLS
#endif