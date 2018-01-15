#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>

namespace smlt {

class Platform {
public:
    // Information
    virtual std::string name() const = 0;

    // Platform-specific utils
    virtual void sleep_ms(uint32_t ms) const = 0;
};

}


#endif // PLATFORM_H
