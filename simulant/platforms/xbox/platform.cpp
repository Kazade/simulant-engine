#include "platform.h"

namespace smlt {

uint64_t XBOXPlatform::available_ram_in_bytes() const {
    return MEMORY_VALUE_UNAVAILABLE;
}

} // namespace smlt
