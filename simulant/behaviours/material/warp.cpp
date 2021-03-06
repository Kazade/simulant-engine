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


#include "./warp.h"

namespace smlt {
namespace behaviours {
namespace material {

void Warp::update(float dt) {
    auto pass = material->pass(0);

    auto tm = pass->diffuse_map_matrix();
    tm[0] = 1.0f + (std::sin(time_) * 0.25f);
    tm[5] = 1.0f + (std::sin(time_) * 0.25f);
    pass->set_diffuse_map_matrix(tm);

    time_ += dt * 0.25f;
}

}
}
}

