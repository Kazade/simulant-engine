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
};

}


#endif // PLATFORM_H
