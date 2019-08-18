/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEIGHTMAP_LOADER_H
#define HEIGHTMAP_LOADER_H

#include <functional>
#include "../loader.h"

namespace smlt {

typedef std::function<smlt::Colour (const smlt::Vec3&, const smlt::Vec3&, const std::vector<Vec3>&)> HeightmapDiffuseGenerator;

struct TerrainData {
    uint32_t x_size;
    uint32_t z_size;
    float max_height;
    float min_height;
    float grid_spacing;
};


namespace terrain {

typedef std::function<void (float height, const Vec3&, float& weight1, float& weight2, float& weight3, float& weight4)> AlphaMapWeightFunc;

void recalculate_terrain_normals(smlt::MeshPtr terrain);
void smooth_terrain(smlt::MeshPtr terrain, uint32_t iterations=20);
TextureID generate_alphamap(smlt::MeshPtr terrain, AlphaMapWeightFunc func);

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
    HeightmapLoader(const unicode& filename, std::shared_ptr<std::istream> data):
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

    virtual ~HeightmapLoaderType() {}

    unicode name() override { return "heightmap_loader"; }
    bool supports(const unicode& filename) const override {
        return filename.lower().ends_with(".tga") || filename.lower().ends_with(".png");
    }

    Loader::ptr loader_for(const unicode& filename, std::shared_ptr<std::istream> data) const override {
        return Loader::ptr(new HeightmapLoader(filename, data));
    }
};

}
}


#endif // HEIGHTMAP_LOADER_H

