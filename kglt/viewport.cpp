#include <kazbase/exceptions.h>

#include "utils/glcompat.h"
#include "viewport.h"
#include "window.h"
#include "utils/gl_error.h"

namespace kglt {

Viewport::Viewport():
    x_(0),
    y_(0),
    width_(1),
    height_(1),
    type_(VIEWPORT_TYPE_CUSTOM),
    colour_(kglt::Colour::BLACK) {

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
    double x, y, width, height;

	GLCheck(glDisable, GL_SCISSOR_TEST);
	switch(type_) {
		case VIEWPORT_TYPE_CUSTOM: {
            x = x_ * target.width();
            y = y_ * target.height();
            width = width_ * target.width();
            height = height_ * target.height();
		}
		break;
		case VIEWPORT_TYPE_FULL: {
			x = 0; y = 0;
            width = target.width();
            height = target.height();
		}
		break;
		case VIEWPORT_TYPE_BLACKBAR_16_BY_9: {
            float desired_height = target.width() / (16.0 / 9.0);
            float y_offset = (target.height() - desired_height) / 2.0;
			x = 0; 
			y = y_offset;
            width = target.width();
			height = desired_height;			
		}
		break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT: {
			x = 0; y = 0;
            width = target.width() / 2.0;
            height = target.height();
		} break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT: {
            x = target.width() / 2.0; y = 0;
            width = target.width() / 2.0;
            height = target.height();
		} break;		
		default:
            throw NotImplementedError(__FILE__, __LINE__);
	}

    GLCheck(glEnable, GL_SCISSOR_TEST);
    GLCheck(glScissor, x, y, width, height);
    GLCheck(glViewport, x, y, width, height);
}

uint32_t Viewport::width_in_pixels(const kglt::RenderTarget& target) const {
    return width_ * target.width();
}

uint32_t Viewport::height_in_pixels(const kglt::RenderTarget& target) const {
    return height_ * target.height();
}


}
