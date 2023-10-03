//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>

#include "color.h"

namespace smlt {

const Color Color::BLACK = Color(0.0, 0.0, 0.0, 1.0);
const Color Color::BLUE = Color(0.0, 0.0, 1.0, 1.0);
const Color Color::GREEN = Color(0.0, 1.0, 0.0, 1.0);
const Color Color::RED = Color(1.0, 0.0, 0.0, 1.0);
const Color Color::WHITE = Color(1.0, 1.0, 1.0, 1.0);
const Color Color::NONE = Color(0, 0, 0, 0); // Transparent
const Color Color::YELLOW = Color(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::PURPLE = Color(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::TURQUOISE = Color(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::GREY = Color(0.5f, 0.5f, 0.5f, 1.0f);

std::string Color::to_hex_string() const {
    auto rval = int(255.0f * r);
    auto gval = int(255.0f * g);
    auto bval = int(255.0f * b);
    auto aval = int(255.0f * a);

    std::string final;

    for(auto& val: {rval, gval, bval, aval}) {
        std::stringstream sstream;
        sstream << std::hex << std::setw(2) << std::setfill('0') << val;
        final += sstream.str();
    }

    return final;
}

Color Color::from_hex_string(const std::string& hex_string) {
    std::string rpart(hex_string.begin(), hex_string.begin() + 2);
    std::string gpart(hex_string.begin() + 2, hex_string.begin() + 4);
    std::string bpart(hex_string.begin() + 4, hex_string.begin() + 6);
    std::string apart(hex_string.begin() + 6, hex_string.end());

    return Color(
        float(strtoul(rpart.c_str(), nullptr, 16)) / 255.0f,
        float(strtoul(gpart.c_str(), nullptr, 16)) / 255.0f,
        float(strtoul(bpart.c_str(), nullptr, 16)) / 255.0f,
        float(strtoul(apart.c_str(), nullptr, 16)) / 255.0f
    );
}

std::ostream& operator<<(std::ostream& stream, const Color& c) {
    stream << "#" << c.to_hex_string();
    return stream;
}

Color Color::lerp(const Color& end, float t) const {
    t = std::min(t, 1.0f);
    t = std::max(t, 0.0f);

    return *this + ((end - *this) * t);
}

PackedColor4444::PackedColor4444():
    color_(~0) {}

PackedColor4444::PackedColor4444(const Color& c):
    color_(
        (uint32_t(15.0f * c.a) << 12) |
        (uint32_t(15.0f * c.r) << 8) |
        (uint32_t(15.0f * c.g) << 4) |
        (uint32_t(15.0f * c.b) << 0)
    ) {

}

PackedColor4444& PackedColor4444::operator=(const Color& rhs) {
    color_ = (
        (uint32_t(15.0f * rhs.a) << 12) |
        (uint32_t(15.0f * rhs.r) << 8) |
        (uint32_t(15.0f * rhs.g) << 4) |
        (uint32_t(15.0f * rhs.b) << 0)
    );

    return *this;
}

bool PackedColor4444::operator==(const PackedColor4444& rhs) const {
    return color_ == rhs.color_;
}

bool PackedColor4444::operator==(const Color& rhs) const {
    auto tmp = PackedColor4444(rhs);
    return *this == tmp;
}

void PackedColor4444::set_alpha(NormalizedFloat a) {
    color_ &= ~(0xF000);
    color_ |= uint32_t(a * 15.0f) << 12;
}

uint8_t PackedColor4444::r8() const {
    /* Unsure if multiplying by 17 is correct, but
     * 15 * 17 == 255, and 15 is the largest value
     * we can store for a channel so it seems right.. */
    return ((color_ & 0x0F00) >> 8) * 17;
}

uint8_t PackedColor4444::g8() const {
    return ((color_ & 0x00F0) >> 4) * 17;
}

uint8_t PackedColor4444::b8() const {
    return ((color_ & 0x000F)) * 17;
}

uint8_t PackedColor4444::a8() const {
    return ((color_ & 0xF000) >> 12) * 17;
}

float PackedColor4444::rf() const {
    float r = r8();
    return r / 255.0f;
}

float PackedColor4444::gf() const {
    return float(g8()) / 255.0f;
}

float PackedColor4444::bf() const {
    return float(b8()) / 255.0f;
}

float PackedColor4444::af() const {
    return float(a8()) / 255.0f;
}

}
