#include <string>
#include <sstream>
#include <iomanip>

#include "colour.h"

namespace smlt {

const Colour Colour::ALICE_BLUE = Colour(0.9375, 0.96875, 0.99609375, 1.0);
const Colour Colour::ANTIQUE_WHITE = Colour(0.9765625, 0.91796875, 0.83984375, 1.0);
const Colour Colour::AQUA = Colour(0.0, 0.99609375, 0.99609375, 1.0);
const Colour Colour::AQUA_MARINE = Colour(0.49609375, 0.99609375, 0.828125, 1.0);
const Colour Colour::AZURE = Colour(0.9375, 0.99609375, 0.99609375, 1.0);
const Colour Colour::BEIGE = Colour(0.95703125, 0.95703125, 0.859375, 1.0);
const Colour Colour::BISQUE = Colour(0.99609375, 0.890625, 0.765625, 1.0);
const Colour Colour::BLACK = Colour(0.0, 0.0, 0.0, 1.0);
const Colour Colour::BLANCHED_ALMOND = Colour(0.99609375, 0.91796875, 0.80078125, 1.0);
const Colour Colour::BLUE = Colour(0.0, 0.0, 0.99609375, 1.0);
const Colour Colour::BLUE_VIOLET = Colour(0.5390625, 0.16796875, 0.8828125, 1.0);
const Colour Colour::BROWN = Colour(0.64453125, 0.1640625, 0.1640625, 1.0);
const Colour Colour::BURLY_WOOD = Colour(0.8671875, 0.71875, 0.52734375, 1.0);
const Colour Colour::CADET_BLUE = Colour(0.37109375, 0.6171875, 0.625, 1.0);
const Colour Colour::CHART_REUSE = Colour(0.49609375, 0.99609375, 0.0, 1.0);
const Colour Colour::CHOCOLATE = Colour(0.8203125, 0.41015625, 0.1171875, 1.0);
const Colour Colour::CORAL = Colour(0.99609375, 0.49609375, 0.3125, 1.0);
const Colour Colour::CORN_FLOWER_BLUE = Colour(0.390625, 0.58203125, 0.92578125, 1.0);
const Colour Colour::CORN_SILK = Colour(0.99609375, 0.96875, 0.859375, 1.0);
const Colour Colour::CRIMSON = Colour(0.859375, 0.078125, 0.234375, 1.0);
const Colour Colour::CYAN = Colour(0.0, 0.99609375, 0.99609375, 1.0);
const Colour Colour::DARK_BLUE = Colour(0.0, 0.0, 0.54296875, 1.0);
const Colour Colour::DARK_CYAN = Colour(0.0, 0.54296875, 0.54296875, 1.0);
const Colour Colour::DARK_GOLDEN_ROD = Colour(0.71875, 0.5234375, 0.04296875, 1.0);
const Colour Colour::DARK_GREEN = Colour(0.0, 0.390625, 0.0, 1.0);
const Colour Colour::DARK_GREY = Colour(0.66015625, 0.66015625, 0.66015625, 1.0);
const Colour Colour::DARK_KHAKI = Colour(0.73828125, 0.71484375, 0.41796875, 1.0);
const Colour Colour::DARK_MAGENTA = Colour(0.54296875, 0.0, 0.54296875, 1.0);
const Colour Colour::DARK_OLIVE_GREEN = Colour(0.33203125, 0.41796875, 0.18359375, 1.0);
const Colour Colour::DARK_ORANGE = Colour(0.99609375, 0.546875, 0.0, 1.0);
const Colour Colour::DARK_ORCHID = Colour(0.59765625, 0.1953125, 0.796875, 1.0);
const Colour Colour::DARK_RED = Colour(0.54296875, 0.0, 0.0, 1.0);
const Colour Colour::DARK_SALMON = Colour(0.91015625, 0.5859375, 0.4765625, 1.0);
const Colour Colour::DARK_SEA_GREEN = Colour(0.55859375, 0.734375, 0.55859375, 1.0);
const Colour Colour::DARK_SLATE_BLUE = Colour(0.28125, 0.23828125, 0.54296875, 1.0);
const Colour Colour::DARK_SLATE_GREY = Colour(0.18359375, 0.30859375, 0.30859375, 1.0);
const Colour Colour::DARK_TURQUOISE = Colour(0.0, 0.8046875, 0.81640625, 1.0);
const Colour Colour::DARK_VIOLET = Colour(0.578125, 0.0, 0.82421875, 1.0);
const Colour Colour::DEEP_PINK = Colour(0.99609375, 0.078125, 0.57421875, 1.0);
const Colour Colour::DEEP_SKY_BLUE = Colour(0.0, 0.74609375, 0.99609375, 1.0);
const Colour Colour::DIM_GREY = Colour(0.41015625, 0.41015625, 0.41015625, 1.0);
const Colour Colour::DODGER_BLUE = Colour(0.1171875, 0.5625, 0.99609375, 1.0);
const Colour Colour::FIREBRICK = Colour(0.6953125, 0.1328125, 0.1328125, 1.0);
const Colour Colour::FLORAL_WHITE = Colour(0.99609375, 0.9765625, 0.9375, 1.0);
const Colour Colour::FOREST_GREEN = Colour(0.1328125, 0.54296875, 0.1328125, 1.0);
const Colour Colour::GAINSBORO = Colour(0.859375, 0.859375, 0.859375, 1.0);
const Colour Colour::GHOST_WHITE = Colour(0.96875, 0.96875, 0.99609375, 1.0);
const Colour Colour::GOLD = Colour(0.99609375, 0.83984375, 0.0, 1.0);
const Colour Colour::GOLDEN_ROD = Colour(0.8515625, 0.64453125, 0.125, 1.0);
const Colour Colour::GREEN = Colour(0.0, 0.5, 0.0, 1.0);
const Colour Colour::GREEN_YELLOW = Colour(0.67578125, 0.99609375, 0.18359375, 1.0);
const Colour Colour::GREY = Colour(0.5, 0.5, 0.5, 1.0);
const Colour Colour::HONEYDEW = Colour(0.9375, 0.99609375, 0.9375, 1.0);
const Colour Colour::HOT_PINK = Colour(0.99609375, 0.41015625, 0.703125, 1.0);
const Colour Colour::INDIAN_RED = Colour(0.80078125, 0.359375, 0.359375, 1.0);
const Colour Colour::INDIGO = Colour(0.29296875, 0.0, 0.5078125, 1.0);
const Colour Colour::IVORY = Colour(0.99609375, 0.99609375, 0.9375, 1.0);
const Colour Colour::KHAKI = Colour(0.9375, 0.8984375, 0.546875, 1.0);
const Colour Colour::LAVENDER = Colour(0.8984375, 0.8984375, 0.9765625, 1.0);
const Colour Colour::LAVENDER_BLUSH = Colour(0.99609375, 0.9375, 0.95703125, 1.0);
const Colour Colour::LAWN_GREEN = Colour(0.484375, 0.984375, 0.0, 1.0);
const Colour Colour::LEMON_CHIFFON = Colour(0.99609375, 0.9765625, 0.80078125, 1.0);
const Colour Colour::LIGHT_BLUE = Colour(0.67578125, 0.84375, 0.8984375, 1.0);
const Colour Colour::LIGHT_CORAL = Colour(0.9375, 0.5, 0.5, 1.0);
const Colour Colour::LIGHT_CYAN = Colour(0.875, 0.99609375, 0.99609375, 1.0);
const Colour Colour::LIGHT_GOLDEN_ROD_YELLOW = Colour(0.9765625, 0.9765625, 0.8203125, 1.0);
const Colour Colour::LIGHT_GREEN = Colour(0.5625, 0.9296875, 0.5625, 1.0);
const Colour Colour::LIGHT_GREY = Colour(0.82421875, 0.82421875, 0.82421875, 1.0);
const Colour Colour::LIGHT_PINK = Colour(0.99609375, 0.7109375, 0.75390625, 1.0);
const Colour Colour::LIGHT_SALMON = Colour(0.99609375, 0.625, 0.4765625, 1.0);
const Colour Colour::LIGHT_SEA_GREEN = Colour(0.125, 0.6953125, 0.6640625, 1.0);
const Colour Colour::LIGHT_SKY_BLUE = Colour(0.52734375, 0.8046875, 0.9765625, 1.0);
const Colour Colour::LIGHT_SLATE_GREY = Colour(0.46484375, 0.53125, 0.59765625, 1.0);
const Colour Colour::LIGHT_STEEL_BLUE = Colour(0.6875, 0.765625, 0.8671875, 1.0);
const Colour Colour::LIGHT_YELLOW = Colour(0.99609375, 0.99609375, 0.875, 1.0);
const Colour Colour::LIME = Colour(0.0, 0.99609375, 0.0, 1.0);
const Colour Colour::LIME_GREEN = Colour(0.1953125, 0.80078125, 0.1953125, 1.0);
const Colour Colour::LINEN = Colour(0.9765625, 0.9375, 0.8984375, 1.0);
const Colour Colour::MAROON = Colour(0.5, 0.0, 0.0, 1.0);
const Colour Colour::MEDIUM_AQUA_MARINE = Colour(0.3984375, 0.80078125, 0.6640625, 1.0);
const Colour Colour::MEDIUM_BLUE = Colour(0.0, 0.0, 0.80078125, 1.0);
const Colour Colour::MEDIUM_ORCHID = Colour(0.7265625, 0.33203125, 0.82421875, 1.0);
const Colour Colour::MEDIUM_PURPLE = Colour(0.57421875, 0.4375, 0.85546875, 1.0);
const Colour Colour::MEDIUM_SEA_GREEN = Colour(0.234375, 0.69921875, 0.44140625, 1.0);
const Colour Colour::MEDIUM_SLATE_BLUE = Colour(0.48046875, 0.40625, 0.9296875, 1.0);
const Colour Colour::MEDIUM_SPRING_GREEN = Colour(0.0, 0.9765625, 0.6015625, 1.0);
const Colour Colour::MEDIUM_TURQUOISE = Colour(0.28125, 0.81640625, 0.796875, 1.0);
const Colour Colour::MEDIUM_VIOLET_RED = Colour(0.77734375, 0.08203125, 0.51953125, 1.0);
const Colour Colour::MIDNIGHT_BLUE = Colour(0.09765625, 0.09765625, 0.4375, 1.0);
const Colour Colour::MINT_CREAM = Colour(0.95703125, 0.99609375, 0.9765625, 1.0);
const Colour Colour::MISTY_ROSE = Colour(0.99609375, 0.890625, 0.87890625, 1.0);
const Colour Colour::MOCCASIN = Colour(0.99609375, 0.890625, 0.70703125, 1.0);
const Colour Colour::NAVAJO_WHITE = Colour(0.99609375, 0.8671875, 0.67578125, 1.0);
const Colour Colour::NAVY = Colour(0.0, 0.0, 0.5, 1.0);
const Colour Colour::OLD_LACE = Colour(0.98828125, 0.95703125, 0.8984375, 1.0);
const Colour Colour::OLIVE = Colour(0.5, 0.5, 0.0, 1.0);
const Colour Colour::OLIVE_DRAB = Colour(0.41796875, 0.5546875, 0.13671875, 1.0);
const Colour Colour::ORANGE = Colour(0.99609375, 0.64453125, 0.0, 1.0);
const Colour Colour::ORANGE_RED = Colour(0.99609375, 0.26953125, 0.0, 1.0);
const Colour Colour::ORCHID = Colour(0.8515625, 0.4375, 0.8359375, 1.0);
const Colour Colour::PALE_GOLDEN_ROD = Colour(0.9296875, 0.90625, 0.6640625, 1.0);
const Colour Colour::PALE_TURQUOISE = Colour(0.68359375, 0.9296875, 0.9296875, 1.0);
const Colour Colour::PALE_VIOLET_RED = Colour(0.85546875, 0.4375, 0.57421875, 1.0);
const Colour Colour::PAPAYA_WHIP = Colour(0.99609375, 0.93359375, 0.83203125, 1.0);
const Colour Colour::PEACH_PUFF = Colour(0.99609375, 0.8515625, 0.72265625, 1.0);
const Colour Colour::PERU = Colour(0.80078125, 0.51953125, 0.24609375, 1.0);
const Colour Colour::PINK = Colour(0.99609375, 0.75, 0.79296875, 1.0);
const Colour Colour::PLUM = Colour(0.86328125, 0.625, 0.86328125, 1.0);
const Colour Colour::POWDER_BLUE = Colour(0.6875, 0.875, 0.8984375, 1.0);
const Colour Colour::PURPLE = Colour(0.5, 0.0, 0.5, 1.0);
const Colour Colour::RED = Colour(0.99609375, 0.0, 0.0, 1.0);
const Colour Colour::ROSY_BROWN = Colour(0.734375, 0.55859375, 0.55859375, 1.0);
const Colour Colour::ROYAL_BLUE = Colour(0.25390625, 0.41015625, 0.87890625, 1.0);
const Colour Colour::SADDLE_BROWN = Colour(0.54296875, 0.26953125, 0.07421875, 1.0);
const Colour Colour::SALMON = Colour(0.9765625, 0.5, 0.4453125, 1.0);
const Colour Colour::SANDY_BROWN = Colour(0.953125, 0.640625, 0.375, 1.0);
const Colour Colour::SEA_GREEN = Colour(0.1796875, 0.54296875, 0.33984375, 1.0);
const Colour Colour::SEA_SHELL = Colour(0.99609375, 0.95703125, 0.9296875, 1.0);
const Colour Colour::SIENNA = Colour(0.625, 0.3203125, 0.17578125, 1.0);
const Colour Colour::SILVER = Colour(0.75, 0.75, 0.75, 1.0);
const Colour Colour::SKY_BLUE = Colour(0.52734375, 0.8046875, 0.91796875, 1.0);
const Colour Colour::SLATE_BLUE = Colour(0.4140625, 0.3515625, 0.80078125, 1.0);
const Colour Colour::SLATE_GREY = Colour(0.4375, 0.5, 0.5625, 1.0);
const Colour Colour::SNOW = Colour(0.99609375, 0.9765625, 0.9765625, 1.0);
const Colour Colour::SPRING_GREEN = Colour(0.0, 0.99609375, 0.49609375, 1.0);
const Colour Colour::STEEL_BLUE = Colour(0.2734375, 0.5078125, 0.703125, 1.0);
const Colour Colour::TAN = Colour(0.8203125, 0.703125, 0.546875, 1.0);
const Colour Colour::TEAL = Colour(0.0, 0.5, 0.5, 1.0);
const Colour Colour::THISTLE = Colour(0.84375, 0.74609375, 0.84375, 1.0);
const Colour Colour::TOMATO = Colour(0.99609375, 0.38671875, 0.27734375, 1.0);
const Colour Colour::TURQUOISE = Colour(0.25, 0.875, 0.8125, 1.0);
const Colour Colour::VIOLET = Colour(0.9296875, 0.5078125, 0.9296875, 1.0);
const Colour Colour::WHEAT = Colour(0.95703125, 0.8671875, 0.69921875, 1.0);
const Colour Colour::WHITE = Colour(0.99609375, 0.99609375, 0.99609375, 1.0);
const Colour Colour::WHITE_SMOKE = Colour(0.95703125, 0.95703125, 0.95703125, 1.0);
const Colour Colour::YELLOW = Colour(0.99609375, 0.99609375, 0.0, 1.0);
const Colour Colour::YELLOW_GREEN = Colour(0.6015625, 0.80078125, 0.1953125, 1.0);

std::string Colour::to_hex_string() const {
    auto rval = int(255.0 * r);
    auto gval = int(255.0 * g);
    auto bval = int(255.0 * b);
    auto aval = int(255.0 * a);

    std::string final;

    for(auto& val: {rval, gval, bval, aval}) {
        std::stringstream sstream;
        sstream << std::hex << std::setw(2) << std::setfill('0') << val;
        final += sstream.str();
    }

    return final;
}

Colour Colour::from_hex_string(const std::string& hex_string) {
    std::string rpart(hex_string.begin(), hex_string.begin() + 2);
    std::string gpart(hex_string.begin() + 2, hex_string.begin() + 4);
    std::string bpart(hex_string.begin() + 4, hex_string.begin() + 6);
    std::string apart(hex_string.begin() + 6, hex_string.end());

    return Colour(
        float(strtoul(rpart.c_str(), nullptr, 16)) / 255.0,
        float(strtoul(gpart.c_str(), nullptr, 16)) / 255.0,
        float(strtoul(bpart.c_str(), nullptr, 16)) / 255.0,
        float(strtoul(apart.c_str(), nullptr, 16)) / 255.0
    );
}

}
