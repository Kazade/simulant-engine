#include "heightmap_loader.h"
#include "../mesh.h"
#include "../resource_manager.h"
#include "texture_loader.h"

namespace kglt {
namespace loaders {

kglt::Colour colour_for_vertex(const kglt::Vec3& point, const kglt::Vec3& normal, const float min, const float max) {
    float height = fabs(min) + point.y;
    float range = fabs(max - min);
    float v = height / range;
    return kglt::Colour(v, v, v, 1.0);
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

    if(options.count("spacing")) {
        spacing = kazbase::any_cast<float>(options.at("spacing"));
    }

    if(options.count("min_height")) {
        min_height = kazbase::any_cast<float>(options.at("min_height"));
    }

    if(options.count("max_height")) {
        max_height = kazbase::any_cast<float>(options.at("max_height"));
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
    kglt::MaterialID mat = mesh->resource_manager().new_material_from_file(Material::BuiltIns::DIFFUSE_ONLY); //mesh->resource_manager().clone_default_material();
    std::vector<kglt::SubMeshID> submeshes;
    for(int i = 0; i < total_patches; ++i) {
        submeshes.push_back(mesh->new_submesh_with_material(mat));
    }

    int32_t height = (int32_t) tex->height();
    int32_t width = (int32_t) tex->width();

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
        mesh->shared_data->diffuse(diffuse_func(pos, n, min_height, max_height));
    }

    for(auto smi: submeshes) {
        mesh->submesh(smi)->index_data->done();
    }
    mesh->shared_data->done();

    mesh->resource_manager().delete_texture(tid); //Finally delete the texture
}

}
}

