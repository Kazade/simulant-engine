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

    if((width & -width) != width || (height & -height) != height) {
        // FIXME: Add SIMULANT_COMPAT_WARNINGS=1 and only do this then
        L_WARN("[COMPAT] Using a non power-of-two texture will break compatibility with some platforms (e.g. Dreamcast)");
    }

    if(data) {
        result.width = (uint32_t) width;
        result.height = (uint32_t) height;
        result.channels = (uint32_t) channels;
        result.data.assign(data, data + (width * height * channels));
        result.texel_type = TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE;

        switch(channels) {
        case 1:
            result.format = TEXTURE_FORMAT_R8;
        break;
        case 2:
            throw std::runtime_error("2-channel textures are not supported");
        break;
        case 3:
            result.format = TEXTURE_FORMAT_RGB888;
        break;
        default:
            result.format = TEXTURE_FORMAT_RGBA8888;
        }

        SOIL_free_image_data(data);
    } else {
        throw std::runtime_error("Unable to load texture: " + filename_.encode());
    }

    return result;
}

}
}
