//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//


#include "./warp.h"

namespace smlt {
namespace controllers {
namespace material {

void Warp::update(double dt) {
    auto pass = material->pass(0);
    auto& tex_unit = pass->texture_unit(0);

    tex_unit.matrix().mat[0] = 1.0 + (sin(time_) * 0.25);
    tex_unit.matrix().mat[5] = 1.0 + (sin(time_) * 0.25);

    time_ += dt * 0.25;
}

}
}
}

