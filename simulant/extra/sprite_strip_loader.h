/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SPRITE_STRIP_LOADER_H
#define SPRITE_STRIP_LOADER_H

#include <cstdint>
#include <vector>
#include <string>
#include "../generic/creator.h"
#include "../types.h"
#include "../resource_manager.h"

namespace smlt {

namespace extra {

class SpriteStripLoader :
        public generic::Creator<SpriteStripLoader> {
public:
    SpriteStripLoader(ResourceManager& rm, const std::string& filename, uint32_t frame_width);
    std::vector<TextureID> load_frames();

private:
    ResourceManager& rm_;
    std::string filename_;
    uint32_t frame_width_;
};

}
}

#endif // SPRITE_STRIP_LOADER_H
