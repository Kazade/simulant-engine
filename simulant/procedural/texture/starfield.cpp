//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "../../texture.h"
#include "../../utils/simplex.h"
#include "../../random.h"

#include "starfield.h"

namespace smlt {
namespace procedural {
namespace texture {

void draw_circle(smlt::TexturePtr texture_ptr, float x, float y, float size, float brightness, const smlt::Colour& colour) {
    smlt::Texture& texture = *texture_ptr;

    float radius = size * 0.5f;

    uint32_t start_y = int(y) - radius - 1;
    uint32_t end_y = int(y) + radius + 1;

    uint32_t start_x = int(x) - radius - 1;
    uint32_t end_x = int(x) + radius + 1;

    int32_t bytes_per_pixel = texture.bytes_per_pixel();

    auto texlock = texture.lock();

    for(uint32_t j = start_y; j < end_y; ++j) {
        for(uint32_t i = start_x; i < end_x; ++i) {
            int dx = x - i;
            int dy = y - j;

            if((dx*dx + dy*dy) <= (radius * radius)) {
                int32_t idx = (j * texture.width()) + i;

                texture.data()[idx * bytes_per_pixel] = brightness * colour.r;
                texture.data()[(idx * bytes_per_pixel) + 1] = brightness * colour.g;
                texture.data()[(idx * bytes_per_pixel) + 2] = brightness * colour.b;
                if(bytes_per_pixel == 4) {
                    texture.data()[(idx * bytes_per_pixel) + 3] = brightness * colour.a;
                }
            }
        }
    }

    texture.mark_data_changed();
}

void starfield(smlt::TexturePtr texture_ptr, uint32_t width, uint32_t height) {
    smlt::Texture& texture = *texture_ptr;

    texture.resize(width, height);
    texture.set_format(TEXTURE_FORMAT_RGBA);

    const float GLOBAL_DENSITY = 0.01f;
    const float MAX_SIZE = 2.0;
    const float MAX_BRIGHTNESS = 255;

    Simplex::ptr noise = Simplex::create(width, height);
    auto rgen = RandomGenerator();

    for(uint32_t y = 0; y < height; ++y) {
        for(uint32_t x = 0; x < width; ++x) {
            float this_density = (noise->get(x, y) + 1.0) / 2.0;

            if(rgen.float_in_range(0, 1) < this_density * GLOBAL_DENSITY) {
                float weight = rgen.float_in_range(0, 1) * this_density;
                float size = std::max(1.0f, weight * MAX_SIZE);
                float brightness = weight * MAX_BRIGHTNESS;

                smlt::Colour colour = smlt::Colour::WHITE;
                float col_rand = rgen.float_in_range(0, 1);
                if(col_rand < 0.05) {
                    colour = smlt::Colour::ORANGE;
                } else if(col_rand < 0.07) {
                    colour = smlt::Colour::YELLOW;
                } else if(col_rand < 0.1) {
                    colour = smlt::Colour::BLUE;
                }
                draw_circle(texture_ptr, x, y, size, brightness, colour);
            }
        }
    }
}

}
}
}

