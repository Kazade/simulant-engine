#ifndef HEIGHTMAP_LOADER_H
#define HEIGHTMAP_LOADER_H

#include <functional>
#include "../loader.h"

namespace kglt {

typedef std::function<kglt::Colour (const kglt::Vec3&, const kglt::Vec3&, const std::vector<Vec3>&)> HeightmapDiffuseGenerator;

namespace loaders {

class HeightmapLoader : public Loader {
public:
    HeightmapLoader(const unicode& filename, std::shared_ptr<std::stringstream> data):
        Loader(filename, data) {}

    void into(Loadable& resource, const LoaderOptions& options = LoaderOptions());

private:
    void smooth_terrain_iteration(Mesh *mesh, int width, int height);
};

class HeightmapLoaderType : public LoaderType {
public:
    HeightmapLoaderType() {
        // Add the mesh hint
        add_hint(LOADER_HINT_MESH);
    }

    ~HeightmapLoaderType() {}

    unicode name() { return "heightmap_loader"; }
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

