#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdint>
#include <string>

namespace smlt {

class Platform {
public:
    // Information
    virtual ~Platform() {}
    virtual std::string name() const = 0;
};

}


#endif // PLATFORM_H
