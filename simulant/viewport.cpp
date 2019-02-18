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

#ifdef _arch_dreamcast
    #include "../deps/libgl/include/gl.h"
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

void Viewport::clear(const RenderTarget &target, uint32_t clear_flags) {
    apply(target);

    GLCheck(glClearColor, colour_.r, colour_.g, colour_.b, colour_.a);

    uint32_t gl_clear_flags = 0;
    if((clear_flags & BUFFER_CLEAR_COLOUR_BUFFER) == BUFFER_CLEAR_COLOUR_BUFFER) {
        gl_clear_flags |= GL_COLOR_BUFFER_BIT;
    }

    if((clear_flags & BUFFER_CLEAR_DEPTH_BUFFER) == BUFFER_CLEAR_DEPTH_BUFFER) {
        gl_clear_flags |= GL_DEPTH_BUFFER_BIT;
    }

    if((clear_flags & BUFFER_CLEAR_STENCIL_BUFFER) == BUFFER_CLEAR_STENCIL_BUFFER) {
        gl_clear_flags |= GL_STENCIL_BUFFER_BIT;
    }

    GLCheck(glClear, gl_clear_flags);
}

void Viewport::apply(const RenderTarget& target) {
    if(type_ != VIEWPORT_TYPE_CUSTOM) {
        calculate_ratios_from_viewport(type_, x_, y_, width_, height_);
    }

	GLCheck(glDisable, GL_SCISSOR_TEST);

    double x = x_ * target.width();
    double y = y_ * target.height();
    double width = width_ * target.width();
    double height = height_ * target.height();

    GLCheck(glEnable, GL_SCISSOR_TEST);
    GLCheck(glScissor, x, y, width, height);
    GLCheck(glViewport, x, y, width, height);
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
            x = 0; width = 1.0; height = (9.0 / 16.0);
            y = (1.0 - height) / 2.0;
        break;
        case VIEWPORT_TYPE_BLACKBAR_4_BY_3:
            x = 0; width = 1.0; height = (3.0 / 4.0);
            y = (1.0 - height) / 2.0;
        break;
        default:
            throw std::logic_error("Unknown viewport mode");
    }
}

}
