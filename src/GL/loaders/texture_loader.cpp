#include <cassert>
#include <stdexcept>
#include <string>
#include <SOIL/SOIL.h>

#include "texture_loader.h"
#include "../texture.h"

namespace GL {
namespace loaders {

void TextureLoader::load_into(Resource& resource, const std::string& filename) {
    Resource* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the TGA loader");

    int width, height, channels;
    unsigned char* data = SOIL_load_image(
        filename.c_str(),
        &width,
        &height,
        &channels,
        SOIL_LOAD_AUTO
    );

    if(!data) {
        throw std::runtime_error("Unable to open file: " + filename);
    }

    tex->set_bpp(channels * 8);
    tex->resize(width, height);
    tex->data().assign(data, data + (width * height * channels));

    SOIL_free_image_data(data);
}

}
}
