#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>

//This is a hack, it should really be fixed in the build system :(
#ifndef __ANDROID__
#include <SOIL/SOIL.h>
#else
#include <SOIL.h>
#endif

#include "texture_loader.h"
#include "../texture.h"

namespace kglt {
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
        SOIL_free_image_data(data);
    }

    return result;
}

}
}
