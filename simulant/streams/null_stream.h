#pragma once

#include "base.h"

namespace smlt {

class NullStream : public Stream {
public:
    virtual StreamState write(const uint8_t*, std::size_t) {
        return status();
    }

    virtual StreamState read(uint8_t*, std::size_t) {
        return status();
    }
};

}
