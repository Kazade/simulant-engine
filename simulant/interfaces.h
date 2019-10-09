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

#ifndef INTERFACES_H
#define INTERFACES_H

#include <vector>
#include "types.h"

namespace smlt {

class RenderTarget {
public:
    virtual ~RenderTarget() {}

    virtual uint16_t width() const = 0;
    virtual uint16_t height() const = 0;

    virtual void set_clear_every_frame(uint32_t clear_flags=BUFFER_CLEAR_ALL, const smlt::Colour& colour=smlt::Colour::BLACK) {
        clear_flags_ = clear_flags;
        clear_colour_ = colour;
    }
    virtual uint32_t clear_every_frame_flags() const { return clear_flags_; }
    virtual smlt::Colour clear_every_frame_colour() const { return clear_colour_; }

private:
    uint32_t clear_flags_ = BUFFER_CLEAR_ALL;
    smlt::Colour clear_colour_ = smlt::Colour::BLACK;
};

}

#endif // INTERFACES_H
