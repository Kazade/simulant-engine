#pragma once

#include "../../platform.h"

namespace smlt {

class AndroidPlatform : public Platform {
public:
    std::string name() const override;

    Resolution native_resolution() const override;

    uint64_t available_ram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t total_ram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t available_vram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override {
        _S_UNUSED(process_id);
        return MEMORY_VALUE_UNAVAILABLE;
    }
};

}
