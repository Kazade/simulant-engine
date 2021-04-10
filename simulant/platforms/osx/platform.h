#pragma once

#include "../../platform.h"

namespace smlt {

class OSXPlatform : public Platform {
public:
    std::string name() const override {
        return "osx";
    }

    Resolution native_resolution() const override {
        SDL_DisplayMode mode;

        Resolution native;
        if(SDL_GetDesktopDisplayMode(0, &mode) == -1) {
            S_WARN("Unable to get the current desktop display mode!!");
            S_WARN("{0}", SDL_GetError());
            S_WARN("Falling back to 1080p");
            native.width = 1920;
            native.height = 1080;
            native.refresh_rate = 60;
        } else {
            native.width = mode.w;
            native.height = mode.h;
            native.refresh_rate = mode.refresh_rate;
        }
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

    uint64_t process_ram_usage_in_bytes(ProcessID process_id) const override {
        _S_UNUSED(process_id);
        return MEMORY_VALUE_UNAVAILABLE;
    }
};

}
