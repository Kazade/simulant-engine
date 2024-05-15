#include "platform.h"

#include <pspsdk.h>

namespace smlt {

uint64_t PSPPlatform::available_ram_in_bytes() const {
    return pspSdkTotalFreeUserMemSize();
}

} // namespace smlt
