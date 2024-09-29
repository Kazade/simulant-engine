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
#include <vector>

#include "generic/range_value.h"
#include "math/utils.h"

namespace smlt {
typedef std::vector<float> FloatArray;

struct Color {
    float r, g, b, a;

    Color() :
        r(0), g(0), b(0), a(0) {}

    Color(const FloatArray& arr) :
        r(arr[0]), g(arr[1]), b(arr[2]), a(arr[3]) {}

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

    Color(float r, float g, float b, float a) :
        r(clamp(r, 0.0f, 1.0f)),
        g(clamp(g, 0.0f, 1.0f)),
        b(clamp(b, 0.0f, 1.0f)),
        a(clamp(a, 0.0f, 1.0f)) {}

    operator FloatArray() const {
        return {r, g, b, a};
    }

    Color operator*(const Color& rhs) const {
        return Color(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a);
    }

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

    uint32_t to_rgba_8888() const {
        uint8_t r = (uint8_t)(this->r * 255.0f);
        uint8_t g = (uint8_t)(this->g * 255.0f);
        uint8_t b = (uint8_t)(this->b * 255.0f);
        uint8_t a = (uint8_t)(this->a * 255.0f);

        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    uint32_t to_argb_8888() const {
        uint8_t r = (uint8_t)(this->r * 255.0f);
        uint8_t g = (uint8_t)(this->g * 255.0f);
        uint8_t b = (uint8_t)(this->b * 255.0f);
        uint8_t a = (uint8_t)(this->a * 255.0f);

        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    uint32_t to_abgr_8888() const {
        uint8_t r = (uint8_t)(this->r * 255.0f);
        uint8_t g = (uint8_t)(this->g * 255.0f);
        uint8_t b = (uint8_t)(this->b * 255.0f);
        uint8_t a = (uint8_t)(this->a * 255.0f);

        return (a << 24) | (b << 16) | (g << 8) | r;
    }

    uint16_t to_rgba_4444() const {
        uint8_t r = (uint8_t)(this->r * 255.0f);
        uint8_t g = (uint8_t)(this->g * 255.0f);
        uint8_t b = (uint8_t)(this->b * 255.0f);
        uint8_t a = (uint8_t)(this->a * 255.0f);

        return ((r >> 4) << 12) | ((g >> 4) << 8) | ((b >> 4) << 4) | (a >> 4);
    }

    uint16_t to_abgr_4444() const {
        uint32_t r = (uint32_t)clamp(this->r * 255.0f, 0, 255);
        uint32_t g = (uint32_t)clamp(this->g * 255.0f, 0, 255);
        uint32_t b = (uint32_t)clamp(this->b * 255.0f, 0, 255);
        uint32_t a = (uint32_t)clamp(this->a * 255.0f, 0, 255);

        return ((a >> 4) << 12) | ((b >> 4) << 8) | ((g >> 4) << 4) | (r >> 4);
    }

    uint16_t to_argb_4444() const {
        uint8_t r = (uint8_t)(this->r * 255.0f);
        uint8_t g = (uint8_t)(this->g * 255.0f);
        uint8_t b = (uint8_t)(this->b * 255.0f);
        uint8_t a = (uint8_t)(this->a * 255.0f);

        return ((a >> 4) << 12) | ((r >> 4) << 8) | ((g >> 4) << 4) | (b >> 4);
    }

    static Color black() {
        return Color(0, 0, 0, 1);
    }
    static Color gray() {
        return Color(0.5, 0.5, 0.5, 1);
    }
    static Color white() {
        return Color(1, 1, 1, 1);
    }
    static Color red() {
        return Color(1, 0, 0, 1);
    }
    static Color green() {
        return Color(0, 1, 0, 1);
    }
    static Color blue() {
        return Color(0, 0, 1, 1);
    }
    static Color yellow() {
        return Color(1, 1, 0, 1);
    }
    static Color purple() {
        return Color(1, 0, 1, 1);
    }
    static Color turquoise() {
        return Color(0, 1, 1, 1);
    }
    static Color none() {
        return Color(0, 0, 0, 0);
    }
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
