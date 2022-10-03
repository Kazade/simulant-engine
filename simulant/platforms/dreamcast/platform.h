#pragma once

#include "../../platform.h"

namespace smlt {

class DreamcastPlatform : public Platform {
public:
    std::string name() const override {
        return "dreamcast";
    }

    Resolution native_resolution() const override {
        Resolution native;
        native.width = 640;
        native.height = 480;
        native.refresh_rate = 60;
        return native;
    }

    uint64_t available_ram_in_bytes() const override;
    uint64_t total_ram_in_bytes() const override;

    uint64_t available_vram_in_bytes() const override;

    uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override;    
};

}
