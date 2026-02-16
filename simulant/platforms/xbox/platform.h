#pragma once

#include "../../platform.h"
#include "simulant/errors.h"

namespace smlt {

class XBOXPlatform : public Platform {
public:
    XBOXPlatform();
    std::string name() const override {
        return "xbox";
    }

    Resolution native_resolution() const override {
        Resolution native;
        native.width = 640;
        native.height = 480;
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
