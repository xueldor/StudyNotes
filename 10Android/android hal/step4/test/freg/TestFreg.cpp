# include <android/hardware/freg/1.0/IFreg.h>

# include <hidl/Status.h>
# include <hidl/LegacySupport.h>
# include <utils/misc.h>
# include <hidl/HidlSupport.h>

# include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

using android::hardware::freg::V1_0::IFreg;
using android::sp;
using android::hardware::hidl_string;

int main()
{
    android::sp<IFreg> service = IFreg::getService();
    if(service == nullptr) {
        printf("Failed to get service\n");
        return -1;
    }

    int val = service->getReg1();
    printf("val = %d\n", val);

    return 0;
}