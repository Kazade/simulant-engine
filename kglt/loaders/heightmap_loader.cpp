#include "heightmap_loader.h"
#include "../mesh.h"
#include "../resource_manager.h"
#include "texture_loader.h"

namespace kglt {
namespace loaders {

inline float clamp(float x, float a, float b) {
    return x < a ? a : (x > b ? b : x);
}

kglt::Colour colour_for_vertex(const kglt::Vec3& point, const kglt::Vec3& normal, const std::vector<Vec3>& surrounding_points) {
    // FIXME: Replace with some kind of decent ambient occlusion
    float sum = 0.0f;

    for(auto& neighbour_pos: surrounding_points) {
        auto neighbour_offset = (neighbour_pos - point).normalized();
        sum += acos(normal.dot(neighbour_offset));
    }

    float v = sum / float(surrounding_points.size());

    // If the average angle > 90 degrees, then we are white
    if(v > 3.142f / 2.0) {
        return kglt::Colour::WHITE;
    } else {
        v /= (3.142 / 2.0);
    }

    return kglt::Colour(v, v, v, 1.0);
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
        results.push_back(data->position_at<Vec3>(idx));
    }

    return results;
}

void HeightmapLoader::smooth_terrain_iteration(Mesh* mesh, int width, int height) {
    mesh->shared_data->move_to_start();
    for(uint32_t i = 0; i < mesh->shared_data->count(); ++i) {
        auto vertices = gather_surrounding_points(mesh->shared_data.get(), width, height, i);

        float total_height = 0.0f;
        for(auto& vert: vertices) {
            total_height += vert.y;
        }

        // http://nic-gamedev.blogspot.co.uk/2013/02/simple-terrain-smoothing.html
        auto this_pos = mesh->shared_data->position_at<Vec3>(i);
        float new_height = ((total_height / float(vertices.size())) + this_pos.y) / 2.0;
        mesh->shared_data->position(this_pos.x, new_height, this_pos.z);
        mesh->shared_data->move_next();
    }
}

void HeightmapLoader::into(Loadable &resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    if(!mesh) {
        throw LogicError("Tried to load a heightmap file into something that wasn't a mesh");
    }

    float spacing = 1.0;
    float min_height = -64.0;
    float max_height = 64.0;
    int smooth_iterations = 15;
    float texture_repeat = 4.0f;

    if(options.count("spacing")) {
        spacing = kazbase::any_cast<float>(options.at("spacing"));
    }

    if(options.count("min_height")) {
        min_height = kazbase::any_cast<float>(options.at("min_height"));
    }

    if(options.count("max_height")) {
        max_height = kazbase::any_cast<float>(options.at("max_height"));
    }

    if(options.count("smooth_iterations")) {
        smooth_iterations = kazbase::any_cast<int>(options.at("smooth_iterations"));
    }

    if(options.count("texture_repeat")) {
        texture_repeat = kazbase::any_cast<float>(options.at("texture_repeat"));
    }

    HeightmapDiffuseGenerator diffuse_func;
    if(options.count("diffuse_func")) {
        diffuse_func = kazbase::any_cast<HeightmapDiffuseGenerator>(options.at("diffuse_func"));
    }

    if(!diffuse_func) {
        diffuse_func = &colour_for_vertex;
    }

    // Load the texture using the texture loader
    TextureID tid = mesh->resource_manager().new_texture();
    TextureLoader loader(this->filename_, this->data_);
    loader.into(*mesh->resource_manager().texture(tid));

    // Now generate the heightmap from it
    auto tex = mesh->resource_manager().texture(tid);
    tex->flip_vertically();

    float range = max_height - min_height;

    float x_offset = (spacing * float(tex->width())) * 0.5f;
    float z_offset = (spacing * float(tex->height())) * 0.5f;

    const int patch_size = 100;

    int patches_across = std::ceil(float(tex->width()) / float(patch_size));
    int patches_down = std::ceil(float(tex->height()) / float(patch_size));

    int total_patches = patches_across * patches_down;

    // We divide the heightmap into patches for more efficient rendering
    kglt::MaterialID mat = mesh->resource_manager().clone_default_material();
    std::vector<kglt::SubMeshID> submeshes;
    for(int i = 0; i < total_patches; ++i) {
        submeshes.push_back(mesh->new_submesh_with_material(mat));
    }

    int32_t height = (int32_t) tex->height();
    int32_t width = (int32_t) tex->width();
    int32_t largest = std::max(width, height);

    // Generate the vertices from the heightmap
    for(int32_t z = 0; z < height; ++z) {
        for(int32_t x = 0; x < width; ++x) {
            int32_t idx = (z * width) + x;

            float height_val = tex->data()[idx * (tex->bpp() / 8)];

            float normalized_height = float(height_val) / float(256.0);
            float depth = range * normalized_height;
            float final_pos = min_height + depth;

            mesh->shared_data->position(
                (float(x) * spacing) - x_offset,
                final_pos,
                (float(z) * spacing) - z_offset
            );
            mesh->shared_data->diffuse(kglt::Colour::WHITE);
            mesh->shared_data->normal(kglt::Vec3(0, 1, 0));

            // First texture coordinate takes into account texture_repeat setting
            mesh->shared_data->tex_coord0(
                (texture_repeat / float(largest)) * float(x),
                (texture_repeat / float(largest)) * float(z)
            );

            // Second texture coordinate makes the texture span the entire terrain
            mesh->shared_data->tex_coord1(
                (1.0 / float(width)) * float(x),
                (1.0 / float(height)) * float(z)
            );

            mesh->shared_data->move_next();

            if(z < (height - 1) && x < (width - 1)) {
                int patch_x = (x / float(patch_size));
                int patch_z = (z / float(patch_size));
                int patch_idx = (patch_z * patches_across) + patch_x;

                auto sm = mesh->submesh(submeshes.at(patch_idx));
                sm->index_data->index(idx);
                sm->index_data->index(idx + width);
                sm->index_data->index(idx + 1);

                sm->index_data->index(idx + 1);
                sm->index_data->index(idx + width);
                sm->index_data->index(idx + width + 1);
            }
        }
    }

    for(auto i = 0; i < smooth_iterations; ++i) {
        smooth_terrain_iteration(mesh, width, height);
    }

    // The mesh don't have any normals, let's generate some!
    std::unordered_map<int, kglt::Vec3> index_to_normal;

    for(auto smi: submeshes) {
        auto sm = mesh->submesh(smi);
        // Go through all the triangles, add the face normal to all the vertices
        for(uint32_t i = 0; i < sm->index_data->count(); i+=3) {
            Index idx1 = sm->index_data->at(i);
            Index idx2 = sm->index_data->at(i+1);
            Index idx3 = sm->index_data->at(i+2);

            kglt::Vec3 v1, v2, v3;
            v1 = sm->vertex_data->position_at<Vec3>(idx1);
            v2 = sm->vertex_data->position_at<Vec3>(idx2);
            v3 = sm->vertex_data->position_at<Vec3>(idx3);

            kglt::Vec3 normal = (v2 - v1).normalized().cross((v3 - v1).normalized()).normalized();

            index_to_normal[idx1] += normal;
            index_to_normal[idx2] += normal;
            index_to_normal[idx3] += normal;
        }
    }

    // Now set the normal on the vertex data
    for(auto p: index_to_normal) {
        mesh->shared_data->move_to(p.first);
        auto n = p.second.normalized();
        Vec3 pos = mesh->shared_data->position_at<Vec3>(p.first);
        mesh->shared_data->normal(n);
        mesh->shared_data->diffuse(
            diffuse_func(pos, n,
                gather_surrounding_points(mesh->shared_data.get(), width, height, p.first)
            )
        );
    }

    for(auto smi: submeshes) {
        mesh->submesh(smi)->index_data->done();
    }
    mesh->shared_data->done();

    mesh->resource_manager().delete_texture(tid); //Finally delete the texture

    // Add some properties for the user to access if they need to
    mesh->stash(uint32_t(width), "terrain_width");
    mesh->stash(uint32_t(height), "terrain_length");
}

}
}

