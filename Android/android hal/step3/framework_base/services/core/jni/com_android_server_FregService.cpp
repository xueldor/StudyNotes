#define LOG_TAG "FregServiceJNI"

#include "jni.h"
#include "nativehelper/JNIHelp.h"
#include "android_runtime/AndroidRuntime.h"

#include <utils/misc.h>
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/freg.h>

#include <stdio.h>

namespace android{
    static void freg_setVal(JNIEnv* env, jclass clazz, jlong ptr, jint value){
        freg_device_t* device = (freg_device_t*)ptr;
        if(!device){
            ALOGE("Device freg is not open");
            return;
        }
        int val = value;
        ALOGI("Set value %d to device freg", val);
        device->set_val(device, val);
    }
    
    static jint freg_getVal(JNIEnv* env, jclass clazz, jlong ptr){
        freg_device_t* device = (freg_device_t*)ptr;
        if(!device){
            ALOGE("Device freg is not open");
            return 0;
        }
        int val = 0;
        device->get_val(device, &val);
        ALOGI("Get value %d from device freg", val);
        return val;
    }

    static inline int freg_device_open(const hw_module_t* module, struct freg_device_t** device){
        return module->methods->open(module, FREG_HARDWARE_DEVICE_ID, (struct hw_device_t**)device);
    }
    static jlong freg_init(JNIEnv* env, jclass clazz){
        freg_module_t* module;
        freg_device_t *device;
        ALOGI("Initializing HAL stub freg....");

        if(hw_get_module(FREG_HARDWARE_MODULE_ID, (const struct hw_module_t**)&module) == 0){
            ALOGI("Device freg found");
            if(freg_device_open(&(module->common), &device) == 0){
                ALOGI("Device freg is open");
                return (jlong)device;
            }
            ALOGE("Failed to open device freg");
            return 0;
        }
        ALOGE("Failed to get HAL stub freg");
        return 0;
    }

    static const JNINativeMethod method_table[] = {
        {"init_native", "()J", (void*)freg_init},
        {"setVal_native", "(JI)V", (void*)freg_setVal},
        {"getVal_native", "(J)I", (void*)freg_getVal}
    };

    int register_android_server_FregService(JNIEnv* env) {
        return jniRegisterNativeMethods(env, "com/android/server/FregService",
            method_table, NELEM(method_table));
    }
}
