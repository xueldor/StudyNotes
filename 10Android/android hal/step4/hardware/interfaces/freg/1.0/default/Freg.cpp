#include "Freg.h"

namespace android {
namespace hardware {
namespace freg {
namespace V1_0 {
namespace implementation {

// Methods from ::android::hardware::freg::V1_0::IFreg follow.
Return<uint64_t> Freg::getReg1() {
    // TODO implement
    return uint64_t(97);
}


// Methods from ::android::hidl::base::V1_0::IBase follow.

IFreg* HIDL_FETCH_IFreg(const char* /* name */) {
    return new Freg();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace freg
}  // namespace hardware
}  // namespace android
