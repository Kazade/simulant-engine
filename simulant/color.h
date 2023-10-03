/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COLOR_H
#define COLOR_H

#include <cstdint>
#include <string>

#include "generic/range_value.h"

namespace smlt {

struct Color {
    float r, g, b, a;

    Color():
        r(1.0), g(1.0), b(1.0), a(1.0) {}

    static Color from_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return Color(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
    }

    /* Read count color components from buf. All other components will be zero
     * except alpha which will be one. */
    Color(const float* buf, std::size_t count):
        r(0), g(0), b(0), a(1) {
        if(count > 0) {
            r = *buf++;
        }
        if(count > 1) {
            g = *buf++;
        }
        if(count > 2) {
            b = *buf++;
        }
        if(count > 3) {
            a = *buf++;
        }
    }

    Color(float r, float g, float b, float a):
        r(r), g(g), b(b), a(a) {}

    Color operator*(const float rhs) const {
        return Color(r * rhs, g * rhs, b * rhs, a * rhs);
    }

    Color operator*=(const float rhs) {
        *this = *this * rhs;
        return *this;
    }

    Color operator-(const Color& rhs) const {
        return Color(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a);
    }

    Color operator-=(const Color& rhs) {
        *this = *this - rhs;
        return *this;
    }

    Color operator+(const Color& rhs) const {
        return Color(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
    }

    Color operator+=(const Color& rhs) {
        *this = *this + rhs;
        return *this;
    }

    bool operator==(const Color& rhs) const {
        return this->r == rhs.r && this->g == rhs.g && this->b == rhs.b && this->a == rhs.a;
    }

    bool operator!=(const Color& rhs) const {
        return this->r != rhs.r || this->g != rhs.g || this->b != rhs.b || this->a != rhs.a;
    }

    bool operator<(const Color& rhs) const {
        if(r < rhs.r) {
            return true;
        } else if (r == rhs.r) {
            if(g < rhs.g) {
                return true;
            } else if(g == rhs.g) {
                if(b < rhs.b) {
                    return true;
                }  else if(b == rhs.b) {
                    if(a < rhs.a) {
                        return true;
                    }
                }
            }
        }

        return false;
    }


    Color lerp(const Color& end, float t) const;

    std::string to_hex_string() const;
    static Color from_hex_string(const std::string& hex_string);

    static const Color BLACK;
    static const Color GREY;
    static const Color WHITE;
    static const Color RED;
    static const Color GREEN;
    static const Color BLUE;
    static const Color YELLOW;
    static const Color PURPLE;
    static const Color TURQUOISE;
    static const Color NONE;

};

/* 16bit packed color in ARGB4444 format. This is useful for minimising
 * ram usage when color fidelity isn't important. */
class PackedColor4444 {
public:
    PackedColor4444();
    PackedColor4444(const Color& c);

    PackedColor4444& operator=(const PackedColor4444&) = default;
    PackedColor4444& operator=(const Color& rhs);

    bool operator==(const PackedColor4444& rhs) const;
    bool operator==(const Color& rhs) const;

    bool operator!=(const Color& rhs) const {
        return !(*this == rhs);
    }

    operator Color() const {
        Color c(rf(), gf(), bf(), af());
        return c;
    }

    void set_alpha(NormalizedFloat a);

    uint8_t r8() const;
    uint8_t g8() const;
    uint8_t b8() const;
    uint8_t a8() const;

    /** Returns the red channel as a value
     * between 0.0f and 1.0f */
    float rf() const;

    /** Returns the green channel as a value
     * between 0.0f and 1.0f */
    float gf() const;

    /** Returns the blue channel as a value
     * between 0.0f and 1.0f */
    float bf() const;

    /** Returns the alpha channel as a value
     * between 0.0f and 1.0f */
    float af() const;

private:
    uint16_t color_;
};


std::ostream& operator<<(std::ostream& stream, const Color& c);
}

#endif // COLOR_H
