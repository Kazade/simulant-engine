#include "../glee/GLee.h"

#include "viewport.h"
#include "window.h"
#include "scene.h"

#include "kazbase/exceptions.h"

namespace kglt {

Viewport::Viewport(Scene* parent):
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

void Viewport::update_opengl() const {
    double x, y, width, height;

	glDisable(GL_SCISSOR_TEST);
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
			width = parent_->window().width();
			height = parent_->window().height();
		}
		break;
		case VIEWPORT_TYPE_BLACKBAR_16_BY_9: {
			glViewport(0, 0, parent_->window().width(), parent_->window().height());
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			
			float desired_height = parent_->window().width() / (16.0 / 9.0);
			float y_offset = (parent_->window().height() - desired_height) / 2.0;
			x = 0; 
			y = y_offset;
			width = parent_->window().width();
			height = desired_height;			
		}
		break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT: {
			x = 0; y = 0;
			width = parent_->window().width() / 2.0;
			height = parent_->window().height();			
		} break;
		case VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT: {
			x = parent_->window().width() / 2.0; y = 0;
			width = parent_->window().width() / 2.0;
			height = parent_->window().height();			
		} break;		
		default:
			assert(0 && "Not Implemented");
	}

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
    glViewport(x, y, width, height);
    glClearColor(colour_.r, colour_.g, colour_.b, colour_.a);
    glClear(GL_COLOR_BUFFER_BIT);
}

}
