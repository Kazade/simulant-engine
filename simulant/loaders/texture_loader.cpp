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

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

#include "../logging.h"

#include "texture_loader.h"
#include "../texture.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG // Enable more verbose error messages
#include "stb_image.h"

namespace smlt {
namespace loaders {

optional<TextureLoadResult> TextureLoader::do_load(std::shared_ptr<FileIfstream> stream) {
    thread::Lock<thread::Mutex> g(lock_); // STB isn't entirely thread-safe

    TextureLoadResult result;

    int width, height, channels;
    unsigned char* data = stbi_load_from_file(
        stream->file(),
        &width,
        &height,
        &channels,
        0
    );

    if((width & -width) != width || (height & -height) != height || width < 8 || height < 8) {
        // FIXME: Add SIMULANT_COMPAT_WARNINGS=1 and only do this then
        S_WARN("[COMPAT] Using a non power-of-two texture will break compatibility with some platforms (e.g. Dreamcast)");
    }

    if(data) {
        result.width = (uint32_t) width;
        result.height = (uint32_t) height;
        result.channels = (uint32_t) channels;
        result.data.assign(data, data + (width * height * channels));

        switch(channels) {
        case 1:
            result.format = TEXTURE_FORMAT_R_1UB_8;
        break;
        case 2:
            S_ERROR("2-channel textures are not supported");
            return optional<TextureLoadResult>();
        break;
        case 3:
            result.format = TEXTURE_FORMAT_RGB_3UB_888;
        break;
        default:
            result.format = TEXTURE_FORMAT_RGBA_4UB_8888;
        }

        stbi_image_free(data);
    } else {
        S_ERROR("Unable to load texture {0}. Reason was {1}",
            filename_,
            stbi_failure_reason()
        );
        return optional<TextureLoadResult>();
    }

    return result;
}

}
}
