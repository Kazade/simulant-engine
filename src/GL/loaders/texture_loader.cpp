#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <SOIL/SOIL.h>

#include "texture_loader.h"
#include "../texture.h"

namespace GL {
namespace loaders {

void TextureLoader::into(Resource& resource) {
    Resource* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the TGA loader");

    int width, height, channels;
    unsigned char* data = SOIL_load_image(
        filename_.c_str(),
        &width,
        &height,
        &channels,
        SOIL_LOAD_AUTO
    );

    if(!data) {
        std::cout << "Falling back to 404.tga" << std::endl;
        data = SOIL_load_image(
            "404.tga",
            &width,
            &height,
            &channels,
            SOIL_LOAD_AUTO
        );
        if(!data) {
            throw std::runtime_error("Couldn't find texture: " + filename_);
        }
    }

    tex->set_bpp(channels * 8);
    tex->resize(width, height);
    tex->data().assign(data, data + (width * height * channels));

    SOIL_free_image_data(data);
}

}
}
