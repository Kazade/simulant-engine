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


#include "./flowing.h"

namespace smlt {
namespace behaviours {
namespace material {

void Flowing::update(float dt) {
    float scroll = -(time_ - int(time_));

    auto pass = material->pass(0);
    auto& tex_unit = *pass->diffuse_map();

    tex_unit.texture_matrix()[12] = scroll;
    tex_unit.texture_matrix()[13] = std::sin(time_ * 5.0f) * 0.125f;

    time_ += (dt * 0.125f);
}

}
}
}
