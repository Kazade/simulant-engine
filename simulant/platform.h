#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string>
#include "types.h"

namespace smlt {

struct Resolution {
    uint16_t width;
    uint16_t height;
    uint16_t refresh_rate;
};

const uint64_t MEMORY_VALUE_UNAVAILABLE = (uint64_t) -1;

class Platform {
public:
    virtual ~Platform() {}

    // Information
    virtual std::string name() const = 0;
    virtual Resolution native_resolution() const = 0;

    /* Return the total amount of RAM in the system */
    virtual uint64_t total_ram_in_bytes() const = 0;

    /*
     * Return the number of bytes of ram available.
     * This may be an approximation. If it's not possible
     * to retreive the info this function returns MEMORY_VALUE_UNAVAILABLE
     */
    virtual uint64_t available_ram_in_bytes() const = 0;

    /*
     * Return the number of used ram in bytes. Returns -1
     * if either total_ram_in_bytes or available_ram_in_bytes is
     * unavailable
     */
    virtual uint64_t used_ram_in_bytes() const {
        auto t = total_ram_in_bytes();
        auto a = available_ram_in_bytes();
        return t - a;
    }

    /* An approximation of the amount of ram used by the specified process
     * in bytes. MEMORY_VALUE_UNAVAILABLE will be returned if unsupported on the platform. For
     * Dreamcast this will be the same as used_ram_in_bytes. */
    virtual uint64_t process_ram_usage_in_bytes(ProcessID process_id) const = 0;

    /*
     * Return the number of bytes of video ram available.
     * This may be an approximation. If it's not possible
     * to retrieve the info this function returns MEMORY_VALUE_UNAVAILABLE
     */
    virtual uint64_t available_vram_in_bytes() const = 0;
};

}


#endif // PLATFORM_H
