#pragma once

#include <cstdint>
#include <cassert>

namespace smlt {

/**
 * Lots of methods have arguments which must be clamped to a range.
 * Using a 'float' or 'int' doesn't convey that via the API, and it
 * means every method must do its own clamping. This template class
 * allows you to specify a range (and type) for arguments and
 * automatically clamps to the range. e.g.
 *
 * void set_some_value(RangeValue<0, 1> value);
 *
 * or
 *
 * void set_some_byte(RangeValue<0, 255, uint8_t> value);
 *
 * Now it's clear what the value should be.
 */
template<int min, int max, typename T=float>
class RangeValue {
public:
    RangeValue() = delete;
    RangeValue(T x):
        value_(clamp(x)) {}

    RangeValue(const RangeValue& rhs):
        value_(rhs.value_) {}

    RangeValue& operator=(const RangeValue& rhs) {
        if(&rhs == this) return *this;

        value_ = rhs.value_;
        return *this;
    }

    operator T() const {
        return value_;
    }

    bool operator==(const RangeValue& rhs) const {
        return value_ == rhs.value_;
    }

    bool operator!=(const RangeValue& rhs) const {
        return !(*this == rhs);
    }

private:
    T clamp(T x) const {
        // Debug assertions
        assert(x >= (T) min);
        assert(x <= (T) max);

        return (x < min) ? (T) min:
               (x > max) ? (T) max : x;
    }

    T value_ = (T) min;
};

typedef RangeValue<0, 1> NormalizedFloat;
typedef RangeValue<0, 255, uint8_t> ByteValue;

}
