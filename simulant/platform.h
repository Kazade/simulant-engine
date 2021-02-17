#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string>

namespace smlt {

struct Resolution {
    uint16_t width;
    uint16_t height;
    uint16_t refresh_rate;
};

class Platform {
public:
    virtual ~Platform() {}

    // Information
    virtual std::string name() const = 0;
    virtual Resolution native_resolution() const = 0;

    /* Return the total amount of RAM in the system */
    virtual int64_t total_ram_in_bytes() const = 0;

    /*
     * Return the number of bytes of ram available.
     * This may be an approximation. If it's not possible
     * to retreive the info this function returns -1
     */
    virtual int64_t available_ram_in_bytes() const = 0;

    /*
     * Return the number of used ram in bytes. Returns -1
     * if either total_ram_in_bytes or available_ram_in_bytes is
     * unavailable
     */
    virtual int64_t used_ram_in_bytes() const {
        auto t = total_ram_in_bytes();
        auto a = available_ram_in_bytes();
        return t - a;
    }

    /* An approximation of the amount of ram used by the current process
     * in bytes. -1 will be returned if unsupported on the platform. For
     * Dreamcast this will be the same as used_ram_in_bytes. */
    virtual int64_t process_ram_usage_in_bytes() const = -1;

    /*
     * Return the number of bytes of video ram available.
     * This may be an approximation. If it's not possible
     * to retreive the info this function returns -1
     */
    virtual int64_t available_vram_in_bytes() const = 0;
};

}


#endif // PLATFORM_H
