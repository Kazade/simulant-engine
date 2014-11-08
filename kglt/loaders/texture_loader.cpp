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

#include <kazbase/exceptions.h>
#include <kazbase/list_utils.h>
#include "../texture.h"

namespace kglt {
namespace loaders {

void TextureLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the texture loader");

    int width, height, channels;
    unsigned char* data = SOIL_load_image(
        filename_.encode().c_str(),
        &width,
        &height,
        &channels,
        SOIL_LOAD_AUTO
    );

    if (!data) {
        throw IOError("Couldn't load the file: " + filename_.encode());
    } else {
        tex->set_bpp(channels * 8);
        tex->resize(width, height);
        tex->data().assign(data, data + (width * height * channels));

        tex->flip_vertically();

        SOIL_free_image_data(data);
    }
}

}
}
