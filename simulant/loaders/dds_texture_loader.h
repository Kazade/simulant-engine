#pragma once

#include "texture_loader.h"

namespace smlt {
namespace loaders {

/*
 * Ideally this wouldn't exist as SOIL does load DDS automatically, however, it doesn't
 * seem like there is any way to read the S3TC format back once it's loaded :(
 */

class DDSTextureLoader : public BaseTextureLoader {
public:
    DDSTextureLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class DDSTextureLoaderType : public LoaderType {
public:
    DDSTextureLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~DDSTextureLoaderType() {}

    unicode name() override { return "dds_texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".dds");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const override {
        return Loader::ptr(new DDSTextureLoader(filename, data));
    }
};

}
}
