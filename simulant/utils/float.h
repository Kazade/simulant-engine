#pragma once

#include <cstdint>

namespace smlt {

struct Float10 {
    static constexpr int max_value = 64512;
    union {
        struct {
            unsigned value : 10;
            unsigned unused : 6;
        } f;
        uint16_t i = 0;
    };
};

optional<Float10> float10_from_float(float input);

float float10_to_float(Float10 f);

} // namespace smlt
