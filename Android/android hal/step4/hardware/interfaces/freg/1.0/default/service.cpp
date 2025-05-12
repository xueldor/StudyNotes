#define LOG_TAG "android.hardware.freg@1.0-service"

#include <android/hardware/freg/1.0/IFreg.h>
#include <hidl/LegacySupport.h>

#include <dlfcn.h>

using android::hardware::freg::V1_0::IFreg;
using android::hardware::defaultPassthroughServiceImplementation;

int main() {
    //直通式
    return defaultPassthroughServiceImplementation<IFreg>();
}