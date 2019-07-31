#pragma once

namespace smlt {

/*
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
    RangeValue(T x):
        value_(clamp(x)) {}

    operator T() const {
        return value_;
    }

private:
    float clamp(T x) const {
        // Debug assertions
        assert(x >= (T) min);
        assert(x <= (T) max);

        return (value_ < min) ? (T) min:
               (value_ > max) ? (T) max : value_;
    }

    T value_;
};

}
