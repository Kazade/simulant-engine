#include <kazbase/exceptions.h>

#include "utils/glcompat.h"
#include "viewport.h"
#include "window.h"
#include "utils/gl_error.h"

namespace kglt {

Viewport::Viewport(WindowBase* parent, ViewportID id):
    generic::Identifiable<ViewportID>(id),
	parent_(parent),
    x_(0),
    y_(0),
    width_(0),
    height_(0),
    type_(VIEWPORT_TYPE_FULL),
    colour_(Colour(0.5, 0.5, 0.5, 0.5)) {
    
}

void Viewport::configure(ViewportType type) {
	type_ = type;
}

void Viewport::set_size(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void Viewport::set_position(uint32_t left, uint32_t top) {
    x_ = left;
    y_ = top;
}

/*
void Viewport::set_perspective_projection(double fov, double near, double far) {
    projection_type_ = PROJECTION_TYPE_PERSPECTIVE;

    kmMat4PerspectiveProjection(&projection_, fov, aspect_, near, far);
}

void Viewport::set_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    projection_type_ = PROJECTION_TYPE_ORTHOGRAPHIC;
    kmMat4OrthographicProjection(&projection_, left, right, bottom, top, near, far);
}

void Viewport::set_orthographic_projection_from_height(float desired_height_in_units) {
    float width = desired_height_in_units * aspect_;
    set_orthographic_projection(-width / 2.0, width / 2.0, -desired_height_in_units / 2.0, desired_height_in_units / 2.0, -10.0, 10.0);
}*/

void Viewport::clear() {
    apply();

    GLCheck(glClearColor, colour_.r, colour_.g, colour_.b, colour_.a);
    GLCheck(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Viewport::apply() const {
    double x, y, width, height;

	GLCheck(glDisable, GL_SCISSOR_TEST);
	switch(type_) {
		case VIEWPORT_TYPE_CUSTOM: {
			x = x_;
			y = y_;
			width = width_;
			height = height_;
		}
		break;
		case VIEWPORT_TYPE_FULL: {
			x = 0; y = 0;
            width = parent_->width();
            height = parent_->height();
		}
		break;
		case VIEWPORT_TYPE_BLACKBAR_16_BY_9: {
            float desired_height = parent_->width() / (16.0 / 9.0);
            float y_offset = (parent_->height() - desired_height) / 2.0;
			x = 0; 
			y = y_offset;
            width = parent_->width();
			height = desired_height;			
		}
		break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT: {
			x = 0; y = 0;
            width = parent_->width() / 2.0;
            height = parent_->height();
		} break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT: {
            x = parent_->width() / 2.0; y = 0;
            width = parent_->width() / 2.0;
            height = parent_->height();
		} break;		
		default:
			assert(0 && "Not Implemented");
	}

    GLCheck(glEnable, GL_SCISSOR_TEST);
    GLCheck(glScissor, x, y, width, height);
    GLCheck(glViewport, x, y, width, height);
}

}
