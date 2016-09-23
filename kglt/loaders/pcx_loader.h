#pragma once


#include "../loader.h"

namespace kglt {
namespace loaders {

class PCXLoader : public BaseTextureLoader {
public:
    PCXLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class PCXLoaderType : public LoaderType {
public:
    PCXLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~PCXLoaderType() {}

    unicode name() { return "pcx_texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".pcx");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new PCXLoader(filename, data));
    }
};

}
}
