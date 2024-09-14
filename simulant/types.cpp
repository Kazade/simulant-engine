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

#include "types.h"
#include "meshes/vertex_format.h"
#include "utils/random.h"

namespace smlt {

const std::vector<RenderPriority> RENDER_PRIORITIES = {
    RENDER_PRIORITY_ABSOLUTE_BACKGROUND,
    RENDER_PRIORITY_BACKGROUND,
    RENDER_PRIORITY_DISTANT,
    RENDER_PRIORITY_MAIN,
    RENDER_PRIORITY_NEAR,
    RENDER_PRIORITY_FOREGROUND,
    RENDER_PRIORITY_ABSOLUTE_FOREGROUND
};



}
