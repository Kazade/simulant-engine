#ifndef WAL_LOADER_H
#define WAL_LOADER_H

/*
 * Texture loader for Quake 2 WAL textures
 */

#include "../loader.h"

namespace smlt {
namespace loaders {

class WALLoader : public BaseTextureLoader {
public:
    WALLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        BaseTextureLoader(filename, data) {}

private:
    TextureLoadResult do_load(const std::vector<uint8_t> &buffer) override;
};

class WALLoaderType : public LoaderType {
public:
    WALLoaderType() {
        // Always add the texture hint
        add_hint(LOADER_HINT_TEXTURE);
    }

    ~WALLoaderType() {}

    unicode name() { return "wal_texture"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().contains(".wal");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new WALLoader(filename, data));
    }
};

}
}

#endif // WAL_LOADER_H
