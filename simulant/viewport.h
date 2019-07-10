/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMULANT_VIEWPORT_H
#define SIMULANT_VIEWPORT_H

#include <memory>
#include <cstdint>
#include "generic/managed.h"
#include "interfaces.h"
#include "types.h"

namespace smlt {

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

class Viewport : public RefCounted<Viewport> {
public:
    Viewport();
    Viewport(const Viewport& rhs) = default;
    Viewport& operator=(const Viewport& rhs) = default;

    Viewport(ViewportType type, const Colour& colour=smlt::Colour::BLACK);
    Viewport(Ratio x, Ratio y, Ratio width, Ratio height, const Colour& colour=smlt::Colour::BLACK);

    Ratio x() const { return x_; }
    Ratio y() const { return y_; }
    Ratio width() const { return width_; }
    Ratio height() const { return height_; }

    void clear(const RenderTarget& target, uint32_t clear_flags);
    void apply(const RenderTarget& target);

    uint32_t width_in_pixels(const RenderTarget& target) const;
    uint32_t height_in_pixels(const RenderTarget& target) const;

    ViewportType type() const { return type_; }

    void set_colour(const smlt::Colour& colour);
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
