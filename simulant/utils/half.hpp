#include <cstdint>

namespace smlt {
class half {
    /* Basic implementation of half-floats until C++23 is widely supported */

public:
    half() :
        data_(0) {}

    half(const float v) :
        data_(float_to_half(v)) {}

    operator float() const {
        return half_to_float(data_);
    }

    half operator*(const half& rhs) const {
        return half_to_float(data_) * half_to_float(rhs.data_);
    }

    half operator/(const half& rhs) const {
        return half_to_float(data_) / half_to_float(rhs.data_);
    }

    half operator+(const half& rhs) const {
        return half_to_float(data_) + half_to_float(rhs.data_);
    }

    half operator-(const half& rhs) const {
        return half_to_float(data_) - half_to_float(rhs.data_);
    }

    half& operator+=(const half& rhs) {
        if(this == &rhs) {
            return *this;
        }

        *this = *this + rhs;
        return *this;
    }

    half& operator-=(const half& rhs) {
        if(this == &rhs) {
            return *this;
        }

        *this = *this - rhs;
        return *this;
    }

    half& operator*=(const half& rhs) {
        if(this == &rhs) {
            return *this;
        }

        *this = *this * rhs;
        return *this;
    }

    half& operator/=(const half& rhs) {
        if(this == &rhs) {
            return *this;
        }

        *this = *this / rhs;
        return *this;
    }

    const uint16_t* data() const {
        return &data_;
    }

private:
    uint16_t data_;

    static uint16_t float_to_half(const float a) {
        uint32_t ia = *((uint32_t*)&a);
        uint16_t ir;

        ir = (ia >> 16) & 0x8000;
        if((ia & 0x7f800000) == 0x7f800000) {
            if((ia & 0x7fffffff) == 0x7f800000) {
                ir |= 0x7c00; /* infinity */
            } else {
                ir |= 0x7e00 | ((ia >> (24 - 11)) & 0x1ff); /* NaN, quietened */
            }
        } else if((ia & 0x7f800000) >= 0x33000000) {
            int shift = (int)((ia >> 23) & 0xff) - 127;
            if(shift > 15) {
                ir |= 0x7c00; /* infinity */
            } else {
                ia = (ia & 0x007fffff) | 0x00800000; /* extract mantissa */
                if(shift < -14) {                    /* denormal */
                    ir |= ia >> (-1 - shift);
                    ia = ia << (32 - (-1 - shift));
                } else { /* normal */
                    ir |= ia >> (24 - 11);
                    ia = ia << (32 - (24 - 11));
                    ir = ir + ((14 + shift) << 10);
                }
                /* IEEE-754 round to nearest of even */
                if((ia > 0x80000000) || ((ia == 0x80000000) && (ir & 1))) {
                    ir++;
                }
            }
        }
        return ir;
    }

    static float half_to_float(const uint16_t h) {
        uint32_t sign = (h & 0x8000) << 16;
        uint32_t exponent = ((h >> 10) & 0x1F);
        uint32_t mantissa = (h & 0x3FF);

        if(exponent == 0) {
            if(mantissa == 0) {
                return *reinterpret_cast<const float*>(&sign); // Zero
            } else {
                // Subnormal
                while((mantissa & 0x400) == 0) {
                    mantissa <<= 1;
                    exponent--;
                }
                exponent++;
                mantissa &= ~0x400;
            }
        } else if(exponent == 31) {
            uint32_t tmp = (sign | 0x7F800000 | (mantissa << 13));
            return *reinterpret_cast<const float*>(&tmp); // Inf or NaN
        }

        exponent = exponent + (127 - 15);
        mantissa = mantissa << 13;

        uint32_t f_bits = sign | (exponent << 23) | mantissa;
        return *reinterpret_cast<const float*>(&f_bits);
    }
};

} // namespace smlt
