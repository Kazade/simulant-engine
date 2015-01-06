#ifndef KGLT_VIEWPORT_H
#define KGLT_VIEWPORT_H

#include <memory>
#include <cstdint>
#include "generic/managed.h"
#include "kazmath/mat4.h"
#include "interfaces.h"
#include "types.h"

namespace kglt {

enum ViewportType {
	VIEWPORT_TYPE_FULL,	
	VIEWPORT_TYPE_BLACKBAR_4_BY_3,
	VIEWPORT_TYPE_BLACKBAR_16_BY_9,
	VIEWPORT_TYPE_BLACKBAR_16_BY_10,
	VIEWPORT_TYPE_VERTICAL_SPLIT_LEFT,
	VIEWPORT_TYPE_VERTICAL_SPLIT_RIGHT,
	VIEWPORT_TYPE_HORIZONTAL_SPLIT_TOP,
	VIEWPORT_TYPE_HORIZONTAL_SPLIT_BOTTOM,
	VIEWPORT_TYPE_CUSTOM
};

void calculate_ratios_from_viewport(ViewportType type, float& x, float& y, float& width, float& height);

class Viewport : public Managed<Viewport> {
public:
    Viewport();
    Viewport(const Viewport& rhs) = default;
    Viewport& operator=(const Viewport& rhs) = default;

    Viewport(ViewportType type, const Colour& colour=kglt::Colour::BLACK);
    Viewport(Ratio x, Ratio y, Ratio width, Ratio height, const Colour& colour=kglt::Colour::BLACK);

    const Ratio x() const { return x_; }
    const Ratio y() const { return y_; }
    const Ratio width() const { return width_; }
    const Ratio height() const { return height_; }

    void clear(const RenderTarget& target, uint32_t clear_flags);
    void apply(const RenderTarget& target);

    uint32_t width_in_pixels(const RenderTarget& target) const;
    uint32_t height_in_pixels(const RenderTarget& target) const;

    ViewportType type() const { return type_; }
private:
    Ratio x_;
    Ratio y_;
    Ratio width_;
    Ratio height_;
    
	ViewportType type_;
	Colour colour_;
};

}

#endif
