#ifndef SIMULANT_TGA_LOADER_H
#define SIMULANT_TGA_LOADER_H

#include "../loader.h"

namespace smlt {
namespace loaders {

class TextureLoader : public BaseTextureLoader {
public:
    TextureLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class TextureLoaderType : public LoaderType {
public:
    TextureLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~TextureLoaderType() {}

    unicode name() { return "texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".tga") || filename.lower().contains(".png") || filename.lower().contains(".jpg") || filename.lower().contains(".dds");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new TextureLoader(filename, data));
    }
};

}
}

#endif
