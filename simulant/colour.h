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

#ifndef COLOUR_H
#define COLOUR_H

#include <cstdint>
#include <string>

namespace smlt {

struct Colour {
    float r, g, b, a;

    Colour():
        r(1.0), g(1.0), b(1.0), a(1.0) {}

    static Colour from_bytes(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        return Colour(float(r) / 255.0f, float(g) / 255.0f, float(b) / 255.0f, float(a) / 255.0f);
    }

    Colour(float r, float g, float b, float a):
        r(r), g(g), b(b), a(a) {}

    Colour operator*(const float rhs) const {
        return Colour(r * rhs, g * rhs, b * rhs, a * rhs);
    }

    Colour operator*=(const float rhs) {
        *this = *this * rhs;
        return *this;
    }

    Colour operator-(const Colour& rhs) const {
        return Colour(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a);
    }

    Colour operator-=(const Colour& rhs) {
        *this = *this - rhs;
        return *this;
    }

    Colour operator+(const Colour& rhs) const {
        return Colour(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
    }

    Colour operator+=(const Colour& rhs) {
        *this = *this + rhs;
        return *this;
    }

    bool operator==(const Colour& rhs) const {
        return this->r == rhs.r && this->g == rhs.g && this->b == rhs.b && this->a == rhs.a;
    }

    bool operator!=(const Colour& rhs) const {
        return this->r != rhs.r || this->g != rhs.g || this->b != rhs.b || this->a != rhs.a;
    }

    bool operator<(const Colour& rhs) const {
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


    Colour lerp(const Colour& end, float t) const;

    std::string to_hex_string() const;
    static Colour from_hex_string(const std::string& hex_string);

    static const Colour BLACK;
    static const Colour GREY;
    static const Colour WHITE;
    static const Colour RED;
    static const Colour GREEN;
    static const Colour BLUE;
    static const Colour YELLOW;
    static const Colour PURPLE;
    static const Colour TURQUOISE;
    static const Colour NONE;

};

std::ostream& operator<<(std::ostream& stream, const Colour& c);
}

#endif // COLOUR_H
