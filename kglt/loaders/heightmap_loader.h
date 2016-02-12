#ifndef HEIGHTMAP_LOADER_H
#define HEIGHTMAP_LOADER_H

#include <functional>
#include "../loader.h"

namespace kglt {
namespace loaders {

class HeightmapLoader : public Loader {
public:
    HeightmapLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());
};

class HeightmapLoaderType : public LoaderType {
public:
    HeightmapLoaderType() {

    }

    ~HeightmapLoaderType() {}

    unicode name() { return "heightmap"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".tga") || filename.lower().ends_with(".png");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::stringstream> data) const {
        return Loader::ptr(new HeightmapLoader(filename, data));
    }
};

}
}


#endif // HEIGHTMAP_LOADER_H

