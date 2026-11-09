#pragma once

#include "../../platform.h"

namespace smlt {

class IOSPlatform : public Platform {
public:
    std::string name() const override;

    Resolution native_resolution() const override;

    uint64_t available_ram_in_bytes() const override;
    uint64_t total_ram_in_bytes() const override;

    uint64_t available_vram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override;

    /* Always empty: iOS doesn't tell apps who the user is (and the device's
     * name is just "iPhone" or "iPad" since iOS 16) */
    User current_user() const override;
};

}
