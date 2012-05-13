#ifndef KGLT_VIEWPORT_H
#define KGLT_VIEWPORT_H

#include <cstdint>
#include "kazmath/mat4.h"
#include "types.h"

namespace kglt {

class Scene;

enum AspectRatio {
    ASPECT_RATIO_CUSTOM,
    ASPECT_RATIO_4_BY_3,
    ASPECT_RATIO_16_BY_9,
    ASPECT_RATIO_16_BY_10
};

enum ProjectionType {
    PROJECTION_TYPE_PERSPECTIVE,
    PROJECTION_TYPE_ORTHOGRAPHIC
};

class Viewport {
public:
    Viewport(Scene* parent);

    void set_size(uint32_t width, uint32_t height);
    void set_position(uint32_t left, uint32_t top);
    void maintain_standard_ratio_size(AspectRatio ratio);
    void maintain_custom_ratio_size(float aspect);

    void set_perspective_projection(double fov, double near=1.0, double far=1000.0f);
    void set_orthographic_projection(double left, double right, double bottom, double top, double near=-1.0, double far=1.0);
    void set_orthographic_projection_from_height(float desired_height_in_units);

    ProjectionType projection_type();

    void update_projection_matrix(kmMat4* pout);
    void update_opengl() const;

	void set_background_colour(const Colour& colour) {
		colour_ = colour;
	}

private:
    uint32_t x_;
    uint32_t y_;
    uint32_t width_;
    uint32_t height_;


    AspectRatio ratio_;
    float aspect_;

	Colour colour_;

    void update_viewport();

    ProjectionType projection_type_;
    kmMat4 projection_;
};

}

#endif
