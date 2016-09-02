#include "deps/kazlog/kazlog.h"
#include "loader.h"
#include "texture.h"

namespace kglt {

Loader::~Loader() {

}

namespace loaders {

void BaseTextureLoader::into(Loadable& resource, const LoaderOptions& options) {
    Loadable* res_ptr = &resource;
    Texture* tex = dynamic_cast<Texture*>(res_ptr);
    assert(tex && "You passed a Resource that is not a texture to the texture loader");

    auto str = this->data_->str();
    std::vector<unsigned char> buffer(str.begin(), str.end());

    auto result = do_load(buffer);

    if (result.data.empty()) {
        L_ERROR(_F("Unable to load texture with name: {0}").format(filename_));
        throw std::runtime_error("Couldn't load the file: " + filename_.encode());
    } else {
        tex->set_bpp(result.channels * 8);
        tex->resize(result.width, result.height);
        tex->data().assign(result.data.begin(), result.data.end());

        tex->flip_vertically();
    }
}

}
}
