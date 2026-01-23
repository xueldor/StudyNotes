#ifndef ANDROID_HARDWARE_FREG_V1_0_FREG_H
#define ANDROID_HARDWARE_FREG_V1_0_FREG_H

#include <android/hardware/freg/1.0/IFreg.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>

namespace android {
namespace hardware {
namespace freg {
namespace V1_0 {
namespace implementation {

using ::android::hardware::hidl_array;
using ::android::hardware::hidl_memory;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::sp;

struct Freg : public IFreg {
    // Methods from ::android::hardware::freg::V1_0::IFreg follow.
    Return<uint64_t> getReg1() override;

    // Methods from ::android::hidl::base::V1_0::IBase follow.

};

// FIXME: most likely delete, this is only for passthrough implementations
extern "C" IFreg* HIDL_FETCH_IFreg(const char* name);

}  // namespace implementation
}  // namespace V1_0
}  // namespace freg
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_FREG_V1_0_FREG_H
