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
        return Colour(float(r) / 255.0, float(g) / 255.0, float(b) / 255.0, float(a) / 255.0);
    }

    Colour(float r, float g, float b, float a):
        r(r), g(g), b(b), a(a) {}

    Colour operator*(const float rhs) const {
        return Colour(r * rhs, g * rhs, b * rhs, a * rhs);
    }

    Colour operator+(const Colour& rhs) const {
        return Colour(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a);
    }

    bool operator==(const Colour& rhs) const {
        return this->r == rhs.r && this->g == rhs.g && this->b == rhs.b && this->a == rhs.a;
    }

    bool operator!=(const Colour& rhs) const {
        return !(*this == rhs);
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

    std::string to_hex_string() const;
    static Colour from_hex_string(const std::string& hex_string);

    static const Colour ALICE_BLUE;
    static const Colour ANTIQUE_WHITE;
    static const Colour AQUA;
    static const Colour AQUA_MARINE;
    static const Colour AZURE;
    static const Colour BEIGE;
    static const Colour BISQUE;
    static const Colour BLACK;
    static const Colour BLANCHED_ALMOND;
    static const Colour BLUE;
    static const Colour BLUE_VIOLET;
    static const Colour BROWN;
    static const Colour BURLY_WOOD;
    static const Colour CADET_BLUE;
    static const Colour CHART_REUSE;
    static const Colour CHOCOLATE;
    static const Colour CORAL;
    static const Colour CORN_FLOWER_BLUE;
    static const Colour CORN_SILK;
    static const Colour CRIMSON;
    static const Colour CYAN;
    static const Colour DARK_BLUE;
    static const Colour DARK_CYAN;
    static const Colour DARK_GOLDEN_ROD;
    static const Colour DARK_GREEN;
    static const Colour DARK_GREY;
    static const Colour DARK_KHAKI;
    static const Colour DARK_MAGENTA;
    static const Colour DARK_OLIVE_GREEN;
    static const Colour DARK_ORANGE;
    static const Colour DARK_ORCHID;
    static const Colour DARK_RED;
    static const Colour DARK_SALMON;
    static const Colour DARK_SEA_GREEN;
    static const Colour DARK_SLATE_BLUE;
    static const Colour DARK_SLATE_GREY;
    static const Colour DARK_TURQUOISE;
    static const Colour DARK_VIOLET;
    static const Colour DEEP_PINK;
    static const Colour DEEP_SKY_BLUE;
    static const Colour DIM_GREY;
    static const Colour DODGER_BLUE;
    static const Colour FIREBRICK;
    static const Colour FLORAL_WHITE;
    static const Colour FOREST_GREEN;
    static const Colour GAINSBORO;
    static const Colour GHOST_WHITE;
    static const Colour GOLD;
    static const Colour GOLDEN_ROD;
    static const Colour GREEN;
    static const Colour GREEN_YELLOW;
    static const Colour GREY;
    static const Colour HONEYDEW;
    static const Colour HOT_PINK;
    static const Colour INDIAN_RED;
    static const Colour INDIGO;
    static const Colour IVORY;
    static const Colour KHAKI;
    static const Colour LAVENDER;
    static const Colour LAVENDER_BLUSH;
    static const Colour LAWN_GREEN;
    static const Colour LEMON_CHIFFON;
    static const Colour LIGHT_BLUE;
    static const Colour LIGHT_CORAL;
    static const Colour LIGHT_CYAN;
    static const Colour LIGHT_GOLDEN_ROD_YELLOW;
    static const Colour LIGHT_GREEN;
    static const Colour LIGHT_GREY;
    static const Colour LIGHT_PINK;
    static const Colour LIGHT_SALMON;
    static const Colour LIGHT_SEA_GREEN;
    static const Colour LIGHT_SKY_BLUE;
    static const Colour LIGHT_SLATE_GREY;
    static const Colour LIGHT_STEEL_BLUE;
    static const Colour LIGHT_YELLOW;
    static const Colour LIME;
    static const Colour LIME_GREEN;
    static const Colour LINEN;
    static const Colour MAROON;
    static const Colour MEDIUM_AQUA_MARINE;
    static const Colour MEDIUM_BLUE;
    static const Colour MEDIUM_ORCHID;
    static const Colour MEDIUM_PURPLE;
    static const Colour MEDIUM_SEA_GREEN;
    static const Colour MEDIUM_SLATE_BLUE;
    static const Colour MEDIUM_SPRING_GREEN;
    static const Colour MEDIUM_TURQUOISE;
    static const Colour MEDIUM_VIOLET_RED;
    static const Colour MIDNIGHT_BLUE;
    static const Colour MINT_CREAM;
    static const Colour MISTY_ROSE;
    static const Colour MOCCASIN;
    static const Colour NAVAJO_WHITE;
    static const Colour NAVY;
    static const Colour OLD_LACE;
    static const Colour OLIVE;
    static const Colour OLIVE_DRAB;
    static const Colour ORANGE;
    static const Colour ORANGE_RED;
    static const Colour ORCHID;
    static const Colour PALE_GOLDEN_ROD;
    static const Colour PALE_TURQUOISE;
    static const Colour PALE_VIOLET_RED;
    static const Colour PAPAYA_WHIP;
    static const Colour PEACH_PUFF;
    static const Colour PERU;
    static const Colour PINK;
    static const Colour PLUM;
    static const Colour POWDER_BLUE;
    static const Colour PURPLE;
    static const Colour RED;
    static const Colour ROSY_BROWN;
    static const Colour ROYAL_BLUE;
    static const Colour SADDLE_BROWN;
    static const Colour SALMON;
    static const Colour SANDY_BROWN;
    static const Colour SEA_GREEN;
    static const Colour SEA_SHELL;
    static const Colour SIENNA;
    static const Colour SILVER;
    static const Colour SKY_BLUE;
    static const Colour SLATE_BLUE;
    static const Colour SLATE_GREY;
    static const Colour SNOW;
    static const Colour SPRING_GREEN;
    static const Colour STEEL_BLUE;
    static const Colour TAN;
    static const Colour TEAL;
    static const Colour THISTLE;
    static const Colour TOMATO;
    static const Colour TURQUOISE;
    static const Colour VIOLET;
    static const Colour WHEAT;
    static const Colour WHITE;
    static const Colour WHITE_SMOKE;
    static const Colour YELLOW;
    static const Colour YELLOW_GREEN;
    static const Colour NONE;

};

std::ostream& operator<<(std::ostream& stream, const Colour& c);

}

#endif // COLOUR_H
