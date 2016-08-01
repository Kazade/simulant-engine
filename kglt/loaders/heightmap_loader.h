#ifndef HEIGHTMAP_LOADER_H
#define HEIGHTMAP_LOADER_H

#include <functional>
#include "../loader.h"

namespace kglt {

typedef std::function<kglt::Colour (const kglt::Vec3&, const kglt::Vec3&, const std::vector<Vec3>&)> HeightmapDiffuseGenerator;

struct TerrainData {
    uint32_t x_size;
    uint32_t z_size;
    float max_height;
    float min_height;
    float grid_spacing;
};


namespace terrain {

typedef std::function<void (float height, const Vec3&, float& weight1, float& weight2, float& weight3, float& weight4)> AlphaMapWeightFunc;

void recalculate_terrain_normals(kglt::MeshPtr terrain);
void smooth_terrain(kglt::MeshPtr terrain, uint32_t iterations=20);
TextureID generate_alphamap(kglt::MeshPtr terrain, AlphaMapWeightFunc func);

}

struct HeightmapSpecification {
    float min_height = -64.0f;
    float max_height = 64.0f;
    float spacing = 2.5f;
    uint32_t smooth_iterations = 0;
    bool calculate_normals = true;
    float texcoord0_repeat = 4.0f;
};

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

