#include "viewport.h"
#include "window.h"
#include "scene.h"

#include "kazbase/exceptions.h"

namespace kglt {

Viewport::Viewport(Scene* parent):
    x_(0),
    y_(0),
    width_(parent ? parent->window().width() : 640),
    height_(parent ? parent->window().height() : 480),
    ratio_(ASPECT_RATIO_16_BY_9),
    aspect_(width_ / height_) {
    
    maintain_standard_ratio_size(ASPECT_RATIO_16_BY_9);
    set_perspective_projection(45.0f);
}
    
void Viewport::set_size(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    update_viewport();
}

void Viewport::set_position(uint32_t left, uint32_t top) {
    x_ = left;
    y_ = top;
    
    update_viewport();
}

void Viewport::maintain_standard_ratio_size(AspectRatio ratio) {
    ratio_ = ratio;
    switch(ratio_) {
        case ASPECT_RATIO_4_BY_3:
            aspect_ = 4.0 / 3.0;
        break;
        case ASPECT_RATIO_16_BY_9:
            aspect_ = 16.0 / 9.0;
        break;        
        case ASPECT_RATIO_16_BY_10:
            aspect_ = 16.0 / 10.0;
        break;
        default:
            throw LogicError("Invalid aspect ratio");
    }
    update_viewport();
}

void Viewport::maintain_custom_ratio_size(float aspect) {
    ratio_ = ASPECT_RATIO_CUSTOM;
    aspect_ = aspect;
    
    update_viewport();
}

void Viewport::set_perspective_projection(double fov, double near, double far) {
    projection_type_ = PROJECTION_TYPE_PERSPECTIVE;
    
    kmMat4PerspectiveProjection(&projection_, fov, aspect_, near, far);
}

void Viewport::set_orthographic_projection(double left, double right, double bottom, double top, double near, double far) {
    projection_type_ = PROJECTION_TYPE_ORTHOGRAPHIC;
    kmMat4OrthographicProjection(&projection_, left, right, bottom, top, near, far);
}

ProjectionType Viewport::projection_type() {
    return projection_type_;
}

void Viewport::update_projection_matrix(kmMat4* pOut) {
    kmMat4Assign(pOut, &projection_);
}

void Viewport::update_opengl() const {
    double x, y, width, height;
    
    width = width_;
    
    switch(ratio_) {
        case ASPECT_RATIO_CUSTOM: {
            height = width_ / aspect_;
        } break;
        case ASPECT_RATIO_4_BY_3:
        case ASPECT_RATIO_16_BY_9:
        case ASPECT_RATIO_16_BY_10: {
            height = width_ / aspect_;
        } break;
        default:
            throw LogicError("Invalid aspect ratio type");
    }

    double diff = height_ - height;
    x = 0.0;
    y = diff / 2.0;
    
    glViewport(x, y, width, height);
}

void Viewport::update_viewport() {

}

}
