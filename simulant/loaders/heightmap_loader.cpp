//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "heightmap_loader.h"
#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "texture_loader.h"

namespace smlt {

namespace terrain {
    // Mesh helper functions specific to heightmaps

float get_height_at(MeshPtr terrain, float x_point, float z_point) {
    /*
     * Returns the interpolated height of the terrain at the specified location
     */

    return 0.0f;
}

Vec3 get_vertex_at(MeshPtr terrain, int x, int z) {
    /* Returns the vertex at the specified point */
    TerrainData data = terrain->data->get<TerrainData>("terrain_data");
    int idx = (z * data.x_size) + x;
    return *terrain->vertex_data->position_at<Vec3>(idx);
}

static std::vector<Vec3> _get_surrounding_vertices_from_index(Mesh* terrain, uint32_t i) {
    TerrainData data = terrain->data->get<TerrainData>("terrain_data");
    uint32_t vertex_count = (data.x_size * data.z_size);

    static std::array<int, 8> surrounding_indexes;
    static std::vector<Vec3> surrounding_vertices;
    surrounding_vertices.clear();
    surrounding_vertices.reserve(8);
    surrounding_indexes.fill(-1);

    bool can_left = i % data.x_size > 1;
    bool can_up = i > data.x_size;
    bool can_right = i % data.x_size < (data.x_size - 1);
    bool can_down = i < vertex_count - data.x_size;

    if(can_up) {
        surrounding_indexes[0] = (i - data.x_size);
        if(can_left) {
            surrounding_indexes[1] = (i - data.x_size - 1);
        }
        if(can_right) {
            surrounding_indexes[2] = (i - data.x_size + 1);
        }
    }

    if(can_left) {
        surrounding_indexes[3] = (i - 1);
    }

    if(can_right) {
        surrounding_indexes[4] = (i + 1);
    }

    if(can_down) {
        surrounding_indexes[5] = (i + data.x_size);
        if(can_left) {
            surrounding_indexes[6] = (i + data.x_size - 1);
        }
        if(can_right) {
            surrounding_indexes[7] = (i + data.x_size + 1);
        }
    }

    VertexData& vertex_data = terrain->vertex_data;

    for(auto& idx: surrounding_indexes) {
        if(idx == -1) continue;
        surrounding_vertices.push_back(*vertex_data.position_at<Vec3>(idx));
    }

    return surrounding_vertices;
}

std::vector<Vec3> get_surrounding_vertices_from_index(MeshPtr terrain, uint32_t i) {
    return _get_surrounding_vertices_from_index(terrain.get(), i);
}

std::vector<Vec3> get_surrounding_vertices(MeshPtr terrain, int x, int z) {
    TerrainData data = terrain->data->get<TerrainData>("terrain_data");
    int32_t i = (z * data.x_size) + x;

    return get_surrounding_vertices_from_index(terrain, i);
}


static void _smooth_terrain_iteration(Mesh* mesh, int width, int height) {
    VertexData& vertex_data = mesh->vertex_data;

    vertex_data.move_to_start();

    for(uint32_t i = 0; i < mesh->vertex_data->count(); ++i) {
        auto this_pos = vertex_data.position_at<Vec3>(i);
        auto vertices = _get_surrounding_vertices_from_index(mesh, i);

        float total_height = this_pos->y;
        for(auto& vert: vertices) {
            total_height += vert.y;
        }

        // http://nic-gamedev.blogspot.co.uk/2013/02/simple-terrain-smoothing.html

        float new_height = total_height / float(vertices.size() + 1);
        vertex_data.position(this_pos->x, new_height, this_pos->z);
        vertex_data.move_next();
    }
}

void smooth_terrain_iteration(MeshPtr mesh, int width, int height) {
    _smooth_terrain_iteration(mesh.get(), width, height);
}

static void _smooth_terrain(Mesh* terrain, uint32_t iterations) {
    TerrainData data = terrain->data->get<TerrainData>("terrain_data");

    for(uint32_t i = 0; i < iterations; ++i) {
        _smooth_terrain_iteration(terrain, data.x_size, data.z_size);
    }
}

void smooth_terrain(MeshPtr terrain, uint32_t iterations) {
    _smooth_terrain(terrain.get(), iterations);
}

}


namespace loaders {

inline float clamp(float x, float a, float b) {
    return x < a ? a : (x > b ? b : x);
}

smlt::Colour colour_for_vertex(const smlt::Vec3& point, const smlt::Vec3& normal, const std::vector<Vec3>& surrounding_points) {
    // FIXME: Replace with some kind of decent ambient occlusion
    float sum = 0.0f;

    for(auto& neighbour_pos: surrounding_points) {
        auto neighbour_offset = (neighbour_pos - point).normalized();
        sum += acos(normal.dot(neighbour_offset));
    }

    float v = sum / float(surrounding_points.size());

    // If the average angle > 90 degrees, then we are white
    if(v > 3.142f / 2.0) {
        return smlt::Colour::WHITE;
    } else {
        v /= (3.142 / 2.0);
    }

    return smlt::Colour(v, v, v, 1.0);
}

std::vector<Vec3> gather_surrounding_points(VertexData* data, int width, int height, uint32_t i) {
    int32_t vertex_count = (width * height);

    std::vector<uint32_t> surrounding_indexes;

    bool can_left = i % (uint32_t)width > 1;
    bool can_up = i > (uint32_t)width;
    bool can_right = i % (uint32_t)width < ((uint32_t)width - 1);
    bool can_down = i < vertex_count - (uint32_t)width;

    if(can_up) {
        surrounding_indexes.push_back(i - width);
        if(can_left) {
            surrounding_indexes.push_back(i - width - 1);
        }
        if(can_right) {
            surrounding_indexes.push_back(i - width + 1);
        }
    }

    if(can_left) {
        surrounding_indexes.push_back(i - 1);
    }

    if(can_right) {
        surrounding_indexes.push_back(i + 1);
    }

    if(can_down) {
        surrounding_indexes.push_back(i + width);
        if(can_left) {
            surrounding_indexes.push_back(i + width - 1);
        }
        if(can_right) {
            surrounding_indexes.push_back(i + width + 1);
        }
    }

    std::vector<Vec3> results;
    for(auto& idx: surrounding_indexes) {
        results.push_back(*data->position_at<Vec3>(idx));
    }

    return results;
}




void HeightmapLoader::into(Loadable &resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    if(!mesh) {
        throw std::logic_error("Tried to load a heightmap file into something that wasn't a mesh");
    }

    HeightmapSpecification spec = (
        options.count("spec") ?
        smlt::any_cast<HeightmapSpecification>(options.at("spec")) :
        HeightmapSpecification()
    );

    // Load the texture using the texture loader
    TexturePtr tex = mesh->asset_manager().new_texture(8, 8, TEXTURE_FORMAT_R8);
    TextureLoader loader(this->filename_, this->data_);    
    loader.into(*tex, {{"auto_upload", false}});

    // Now generate the heightmap from it
    tex->flip_vertically();

    if(tex->is_compressed()) {
        throw std::logic_error("Creating a heightmap from a compressed texture is currently unimplemented");
    }

    float range = spec.max_height - spec.min_height;

    float x_offset = (spec.spacing * float(tex->width())) * 0.5f;
    float z_offset = (spec.spacing * float(tex->height())) * 0.5f;

    const int patch_size = 100;

    int patches_across = std::ceil(float(tex->width()) / float(patch_size));
    int patches_down = std::ceil(float(tex->height()) / float(patch_size));

    int total_patches = patches_across * patches_down;

    auto index_type = (tex->width() * tex->height() > std::numeric_limits<uint16_t>::max()) ?
        INDEX_TYPE_32_BIT : INDEX_TYPE_16_BIT;

    // We divide the heightmap into patches for more efficient rendering
    smlt::MaterialID mat = mesh->asset_manager().clone_default_material();
    std::vector<smlt::SubMesh*> submeshes;
    for(int i = 0; i < total_patches; ++i) {
        submeshes.push_back(mesh->new_submesh_with_material(
            std::to_string(i), mat, MESH_ARRANGEMENT_TRIANGLES, index_type
        ));
        submeshes.back()->index_data->reserve(patch_size * patch_size);
    }

    int32_t height = (int32_t) tex->height();
    int32_t width = (int32_t) tex->width();
    int32_t largest = std::max(width, height);
    int32_t total = width * height;

    std::vector<float> heights(total);
    auto& tex_data = tex->data();
    auto stride = tex->bytes_per_pixel();
    for(int32_t i = 0; i < total; i++) {
        heights[i] = float(tex_data[i * stride]) / 256.0f;
    }

    // Add some properties for the user to access if they need to
    TerrainData data;
    data.x_size = width;
    data.z_size = height;
    data.min_height = spec.min_height;
    data.max_height = spec.max_height;
    data.grid_spacing = spec.spacing;
    mesh->data->stash(data, "terrain_data");

    // Generate the vertices from the heightmap
    for(int32_t z = 0; z < height; ++z) {
        for(int32_t x = 0; x < width; ++x) {
            int32_t idx = (z * width) + x;

            float normalized_height = heights[idx];
            float depth = range * normalized_height;
            float final_pos = spec.min_height + depth;

            Vec3 pos = Vec3(
                (float(x) * spec.spacing) - x_offset,
                final_pos,
                (float(z) * spec.spacing) - z_offset
            );
            mesh->vertex_data->position(pos);
            mesh->vertex_data->normal(Vec3(0, 1, 0));

            mesh->vertex_data->diffuse(smlt::Colour::WHITE);

            // First texture coordinate takes into account texture_repeat setting
            mesh->vertex_data->tex_coord0(
                (spec.texcoord0_repeat / float(largest)) * float(x),
                (spec.texcoord0_repeat / float(largest)) * float(z)
            );

            // Second texture coordinate makes the texture span the entire terrain
            mesh->vertex_data->tex_coord1(
                (1.0 / float(width)) * float(x),
                (1.0 / float(height)) * float(z)
            );

            mesh->vertex_data->move_next();

            if(z < (height - 1) && x < (width - 1)) {
                int patch_x = (x / float(patch_size));
                int patch_z = (z / float(patch_size));
                int patch_idx = (patch_z * patches_across) + patch_x;

                auto sm = submeshes.at(patch_idx);
                sm->index_data->index(idx);
                sm->index_data->index(idx + width);
                sm->index_data->index(idx + 1);

                sm->index_data->index(idx + 1);
                sm->index_data->index(idx + width);
                sm->index_data->index(idx + width + 1);
            }
        }
    }

    if(spec.smooth_iterations) {
        terrain::_smooth_terrain(mesh, spec.smooth_iterations);
    }

    if(spec.calculate_normals) {
        // The mesh don't have any normals, let's generate some!
        std::unordered_map<int, smlt::Vec3> index_to_normal;

        for(auto sm: submeshes) {
            // Go through all the triangles, add the face normal to all the vertices
            for(uint32_t i = 0; i < sm->index_data->count(); i+=3) {
                Index idx1 = sm->index_data->at(i);
                Index idx2 = sm->index_data->at(i+1);
                Index idx3 = sm->index_data->at(i+2);

                auto v1 = sm->vertex_data->position_at<Vec3>(idx1);
                auto v2 = sm->vertex_data->position_at<Vec3>(idx2);
                auto v3 = sm->vertex_data->position_at<Vec3>(idx3);

                smlt::Vec3 normal = (*v2 - *v1).normalized().cross((*v3 - *v1).normalized()).normalized();

                index_to_normal[idx1] += normal;
                index_to_normal[idx2] += normal;
                index_to_normal[idx3] += normal;
            }
        }

        // Now set the normal on the vertex data
        for(auto p: index_to_normal) {
            mesh->vertex_data->move_to(p.first);
            auto n = p.second.normalized();
            mesh->vertex_data->normal(n);
        }
    }

    for(auto sm: submeshes) {
        sm->index_data->done();
    }
    mesh->vertex_data->done();

    mesh->asset_manager().destroy_texture(tex->id()); //Finally delete the texture
}

}
}

