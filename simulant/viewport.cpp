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

#include "viewport.h"
#include "utils/gl_error.h"

#ifdef __DREAMCAST__
    #include "../deps/libgl/include/GL/gl.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "renderers/glad/glad/glad.h"
#endif

namespace smlt {

Viewport::Viewport():
    x_(0),
    y_(0),
    width_(1),
    height_(1),
    type_(VIEWPORT_TYPE_CUSTOM),
    colour_(smlt::Colour::BLACK) {

}

Viewport::Viewport(ViewportType type, const Colour& colour):
    x_(0),
    y_(0),
    width_(0),
    height_(0),
    type_(type),
    colour_(colour) {

    calculate_ratios_from_viewport(type_, x_, y_, width_, height_);
}

Viewport::Viewport(Ratio x, Ratio y, Ratio width, Ratio height, const Colour &colour):
    x_(x),
    y_(y),
    width_(width),
    height_(height),
    type_(VIEWPORT_TYPE_CUSTOM),
    colour_(colour) {

}

void Viewport::set_colour(const smlt::Colour& colour) {
    colour_ = colour;
}

uint32_t Viewport::width_in_pixels(const smlt::RenderTarget& target) const {
    return width_ * target.width();
}

uint32_t Viewport::height_in_pixels(const smlt::RenderTarget& target) const {
    return height_ * target.height();
}

void calculate_ratios_from_viewport(ViewportType type, float& x, float& y, float& width, float& height) {
    switch(type) {
        case VIEWPORT_TYPE_FULL:
            x = 0; y = 0; width = 1.0; height = 1.0;
        break;
        case VIEWPORT_TYPE_HORIZONTAL_SPLIT_BOTTOM:
            x = 0; y = 0.0; width = 1.0; height = 0.5;
        break;
        case VIEWPORT_TYPE_HORIZONTAL_SPLIT_TOP:
            x = 0; y = 0.5; width = 1.0; height = 0.5;
        break;
        case VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT:
            x = 0; y = 0; width = 0.5; height = 1.0;
        break;
        case VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT:
            x = 0.5; y = 0; width = 0.5; height = 1.0;
        break;
        case VIEWPORT_TYPE_BLACKBAR_16_BY_9:
            x = 0; width = 1.0; height = (9.0f / 16.0f);
            y = (1.0f - height) / 2.0f;
        break;
        case VIEWPORT_TYPE_BLACKBAR_4_BY_3:
            x = 0; width = 1.0; height = (3.0f / 4.0f);
            y = (1.0f - height) / 2.0f;
        break;
        default:
            throw std::logic_error("Unknown viewport mode");
    }
}

}
