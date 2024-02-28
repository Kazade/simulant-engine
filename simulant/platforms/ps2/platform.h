#pragma once

#include "../../platform.h"

namespace smlt {

class PSPPlatform : public Platform {
public:
    std::string name() const override {
        return "psp";
    }

    Resolution native_resolution() const override {
        Resolution native;
        native.width = 480;
        native.height = 272;
        native.refresh_rate = 60; // FIXME?
        return native;
    }

    uint64_t available_ram_in_bytes() const override {
        return MEMORY_VALUE_UNAVAILABLE;
    }

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
