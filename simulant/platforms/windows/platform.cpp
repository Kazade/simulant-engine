#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>

#include <SDL.h>

#include "platform.h"

namespace smlt {

uint64_t WindowsPlatform::total_ram_in_bytes() const {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullTotalPhys;
}

uint64_t WindowsPlatform::available_vram_in_bytes() const {
    return MEMORY_VALUE_UNAVAILABLE;
}

uint64_t WindowsPlatform::process_ram_usage_in_bytes(ProcessID process_id) const {
    _S_UNUSED(process_id);

    PROCESS_MEMORY_COUNTERS_EX pmc;
    // FIXME: Use OpenProcess(process_id)
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    return pmc.PrivateUsage;
}

Resolution WindowsPlatform::native_resolution() const {
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

uint64_t WindowsPlatform::available_ram_in_bytes() const {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo.ullAvailPhys;
}

}
