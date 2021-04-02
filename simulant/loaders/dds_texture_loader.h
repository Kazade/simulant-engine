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
    DDSTextureLoader(const Path& filename, std::shared_ptr<std::istream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(std::shared_ptr<FileIfstream> stream) override;
};

class DDSTextureLoaderType : public LoaderType {
public:
    DDSTextureLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    virtual ~DDSTextureLoaderType() {}

    const char* name() override { return "dds_texture"; }
    bool supports(const Path& filename) const override {
        return filename.ext() == ".dds";
    }

    Loader::ptr loader_for(const Path& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new DDSTextureLoader(filename, data));
    }
};

}
}
