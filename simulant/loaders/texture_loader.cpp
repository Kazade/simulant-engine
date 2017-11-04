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

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../deps/SOIL/SOIL.h"

#include "texture_loader.h"
#include "../texture.h"

namespace smlt {
namespace loaders {

TextureLoadResult TextureLoader::do_load(const std::vector<uint8_t> &buffer) {
    TextureLoadResult result;

    int width, height, channels;
    unsigned char* data = SOIL_load_image_from_memory(
        &buffer[0],
        buffer.size(),
        &width,
        &height,
        &channels,
        SOIL_LOAD_AUTO
    );

    if(data) {
        result.width = (uint32_t) width;
        result.height = (uint32_t) height;
        result.channels = (uint32_t) channels;
        result.data.assign(data, data + (width * height * channels));
        result.texel_type = TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE;
        result.format = (channels == 4) ? TEXTURE_FORMAT_RGBA : TEXTURE_FORMAT_RGB;
        SOIL_free_image_data(data);
    }

    return result;
}

}
}
