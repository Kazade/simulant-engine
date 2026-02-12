#pragma once

#include "../../platform.h"

namespace smlt {

class XBOXPlatform : public Platform {
public:
    std::string name() const override {
        return "xbox";
    }

    Resolution native_resolution() const override {
        /* This is the highest progressive scan, maybe go 1080i? */
        Resolution native;
        native.width = 1280;
        native.height = 720;
        native.refresh_rate = 60;
        return native;
    }

    uint64_t available_ram_in_bytes() const override;

    uint64_t total_ram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t available_vram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

    uint64_t process_ram_usage_in_bytes(ProcessID) const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }
};


}
