#include "heightmap_loader.h"
#include "../mesh.h"
#include "../resource_manager.h"
#include "texture_loader.h"

namespace kglt {
namespace loaders {

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

    // Load the texture using the texture loader
    TextureID tid = mesh->resource_manager().new_texture();
    TextureLoader loader(this->filename_, this->data_);
    loader.into(*mesh->resource_manager().texture(tid).__object);

    // Now generate the heightmap from it
    auto tex = mesh->resource_manager().texture(tid);

    float range = max_height - min_height;

    float x_offset = (spacing * float(tex->width())) * 0.5f;
    float z_offset = (spacing * float(tex->height())) * 0.5f;

    const int patch_size = 100;

    int patches_across = tex->width() / patch_size;
    if(tex->width() % patch_size) {
        patches_across++;
    }

    int patches_down = tex->height() / patch_size;
    if(tex->height() % patch_size) {
        patches_down++;
    }

    int total_patches = patches_across * patches_down;

    // We divide the heightmap into patches for more efficient rendering
    kglt::MaterialID mat = mesh->resource_manager().clone_default_material();
    std::vector<kglt::SubMeshIndex> submeshes;
    for(int i = 0; i < total_patches; ++i) {
        submeshes.push_back(mesh->new_submesh(mat));
    }

    // Generate the vertices from the heightmap
    for(uint32_t z = 0; z < tex->height(); ++z) {
        for(uint32_t x = 0; x < tex->width(); ++x) {
            uint32_t idx = (z * tex->width()) + x;

            float normalized_height = float(tex->data()[idx]) / float(256.0);
            float height = range * normalized_height;
            float final_pos = min_height + height;

            mesh->shared_data().position(
                (x * spacing) - x_offset,
                final_pos,
                (z * spacing) - z_offset
            );
            mesh->shared_data().move_next();

            if(z < tex->height() - 1 && x < tex->width() - 1) {
                int patch_x = x / patches_across;
                int patch_z = z / patches_down;
                int patch_idx = (patch_z * patches_across) + patch_x;

                auto& sm = mesh->submesh(submeshes.at(patch_idx));
                sm.index_data().index(idx);
                sm.index_data().index(idx + tex->width());
                sm.index_data().index(idx + 1);

                sm.index_data().index(idx + 1);
                sm.index_data().index(idx + tex->width());
                sm.index_data().index(idx + tex->width() + 1);
            }
        }
    }
    mesh->shared_data().done();

    mesh->resource_manager().delete_texture(tid); //Finally delete the texture
}

}
}

