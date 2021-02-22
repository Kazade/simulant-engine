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
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include "./deps/tiny_obj_loader.h"

#include "obj_loader.h"

#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../shortcuts.h"
#include "../vfs.h"
#include "../utils/string.h"

namespace smlt {
namespace loaders {

class SimulantMaterialReader : public tinyobj::MaterialReader {
public:
    SimulantMaterialReader(VirtualFileSystem* locator, const unicode& obj_filename):
        locator_(locator),
        obj_filename_(obj_filename) {}

    bool operator()(const std::string &matId,
        std::vector<tinyobj::material_t> *materials,
        std::map<std::string, int> *matMap, std::string *warn,
        std::string *err) override {

        std::string filename = kfs::path::join(kfs::path::dir_name(obj_filename_.encode()), matId);

        try {
            auto stream = locator_->open_file(filename);
            tinyobj::LoadMtl(matMap, materials, stream.get(), warn, err);
        } catch(AssetMissingError& e) {
            S_DEBUG("mtllib {0} not found. Skipping.", filename);
        }

        return true;
    }

private:
    VirtualFileSystem* locator_ = nullptr;
    unicode obj_filename_;
};

void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);

    S_DEBUG("Loading mesh from {0}", filename_);

    MeshLoadOptions mesh_opts;
    auto it = options.find(MESH_LOAD_OPTIONS_KEY);

    if(it != options.end()) {
        mesh_opts = smlt::any_cast<MeshLoadOptions>(it->second);
    }

    S_DEBUG("Got MeshOptions");

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    S_DEBUG("About to read the obj model");

    SimulantMaterialReader reader(vfs.get(), filename_);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, data_.get(), &reader);

    if(!ret) {
        S_ERROR("Unable to load .obj file {0}", filename_);
        S_ERROR("Error was: {0}", err);
        return;
    }

    if(!warn.empty()) {
        S_WARN(warn);
    }

    S_DEBUG("Mesh has {0} shapes and {1} materials", shapes.size(), materials.size());

    VertexSpecification spec = mesh->vertex_data->vertex_specification();
    mesh->reset(spec);  // Make sure we're empty before we begin

    std::unordered_map<std::string, MaterialPtr> final_materials;
    std::unordered_map<int32_t, SubMeshPtr> material_submeshes;
    std::unordered_map<std::string, TexturePtr> loaded_textures;

    auto index_type = (attrib.vertices.size() / 3 >= std::numeric_limits<uint16_t>::max()) ?
        INDEX_TYPE_32_BIT : INDEX_TYPE_16_BIT;

    // First, load the materials, and create submeshes
    uint32_t i = 0;
    for(auto& material: materials) {
        MaterialPtr new_mat = mesh->asset_manager().clone_default_material();

        auto alpha = (material.dissolve) ? 1.0f : 0.0f;

        new_mat->each([&](uint32_t, MaterialPass* pass) {
            pass->set_diffuse(smlt::Colour(material.diffuse[0], material.diffuse[1], material.diffuse[2], alpha));
            pass->set_ambient(smlt::Colour(material.ambient[0], material.ambient[1], material.ambient[2], alpha));
            pass->set_specular(smlt::Colour(material.specular[0], material.specular[1], material.specular[2], alpha));

            // Shininess values "normally" are between 0 and 1000, but OpenGL expects them to
            // be up to 128 so we scale that here
            pass->set_shininess((material.shininess / 1000.0f) * 128);
            pass->set_cull_mode(mesh_opts.cull_mode);

            if(!mesh_opts.blending_enabled) {
                pass->set_blend_func(smlt::BLEND_NONE);
            }
        });

        /* Apply the diffuse texture (if any) */
        if(!material.diffuse_texname.empty()) {
            auto it = loaded_textures.find(material.diffuse_texname);
            if(it != loaded_textures.end()) {
                new_mat->set_diffuse_map(it->second);
            } else {
                std::vector<std::string> possible_locations;

                // Check relative texture file first
                possible_locations.push_back(
                    kfs::path::join(
                        kfs::path::dir_name(filename_.encode()),
                        material.diffuse_texname
                    )
                );

                // Check potentially absolute file path
                possible_locations.push_back(material.diffuse_texname);

                bool found = false;
                for(auto& texture_file: possible_locations) {
                    if(kfs::path::exists(texture_file)) {
                        auto tex = mesh->asset_manager().new_texture_from_file(texture_file);
                        new_mat->set_diffuse_map(tex);
                        loaded_textures.insert(std::make_pair(material.diffuse_texname, tex));
                        found = true;
                        break;
                    }
                }

                if(!found) {
                    S_WARN("Unable to locate texture {0}", material.diffuse_texname);
                }
            }
        }

        final_materials.insert(std::make_pair(material.name, new_mat));

        auto submesh = mesh->new_submesh_with_material(
            material.name, new_mat->id(), MESH_ARRANGEMENT_TRIANGLES, index_type
        );
        material_submeshes.insert(std::make_pair(i++, submesh));
    }

    S_DEBUG("Loaded materials for obj model");

    typedef std::tuple<int, int, int> VertexKey;

    std::unordered_map<VertexKey, uint32_t> shared_vertices;

    float default_tc [] = {0.0f, 0.0f};
    float default_n [] = {0.0f, 0.0f, 1.0f};

    for(auto& shape: shapes) {
        uint32_t offset = 0;
        for(uint32_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            uint8_t num_verts = shape.mesh.num_face_vertices[f];
            assert(num_verts == 3 && "Only triangles supported");

            auto mat_id = shape.mesh.material_ids[f];
            if(mat_id == -1 && !material_submeshes.count(mat_id)) {
                // Special case, no material!
                material_submeshes.insert(
                    std::make_pair(
                        mat_id,
                        mesh->new_submesh("__default__", MESH_ARRANGEMENT_TRIANGLES, index_type)
                    )
                );
            }

            if(!material_submeshes.count(mat_id)) {
                S_ERROR("Unable to find submesh with mat id: {0}", mat_id);
            }

            auto submeshptr = material_submeshes.at(mat_id);

            for(auto i = 0; i < num_verts; ++i) {
                auto index = shape.mesh.indices[offset + i];
                auto key = std::make_tuple(
                    index.vertex_index, index.normal_index, index.texcoord_index
                );

                /* If the material has a diffuse texture, and no texture, then ignore
                 * if that's what's requested.
                 * FIXME: Otherwise, just ignore the texture as per spec */
                if(!mesh_opts.obj_include_faces_with_missing_texture_vertices &&
                    index.texcoord_index == -1 && mat_id != -1 &&
                    !materials[mat_id].diffuse_texname.empty()) {
                    break;
                }

                auto it = shared_vertices.find(key);
                if(it == shared_vertices.end()) {
                    float* pos = &attrib.vertices[3 * index.vertex_index];
                    float* colour = &attrib.colors[3 * index.vertex_index];
                    float* tc = (index.texcoord_index == -1) ?
                        &default_tc[0] : &attrib.texcoords[2 * index.texcoord_index];
                    float* n = (index.normal_index == -1) ?
                        &default_n[0] : &attrib.normals[3 * index.normal_index];

                    mesh->vertex_data->position(pos[0], pos[1], pos[2]);

                    /* Tinyobj loader loads the non-standard vertex colour extension
                     * but defaults to white anyway so it's safe to just read them in */
                    if(spec.has_diffuse()) {
                        mesh->vertex_data->diffuse(
                            smlt::Colour(colour[0], colour[1], colour[2], 1.0)
                        );
                    }

                    if(spec.has_normals()) {
                        mesh->vertex_data->normal(n[0], n[1], n[2]);
                    }

                    if(spec.has_texcoord0()) {
                        mesh->vertex_data->tex_coord0(tc[0], tc[1]);
                    }

                    mesh->vertex_data->move_next();

                    auto idx = mesh->vertex_data->count() - 1;
                    shared_vertices.insert(std::make_pair(key, idx));

                    submeshptr->index_data->index(idx);
                } else {
                    submeshptr->index_data->index(it->second);
                }
            }

            offset += num_verts;
            submeshptr->index_data->done();
        }
    }

    S_DEBUG("Loaded shapes for obj model");

    std::vector<std::string> empty;
    for(auto submesh: mesh->each_submesh()) {
        if(!submesh->index_data->count()) {
            empty.push_back(submesh->name());
        }
    }

    for(auto& name: empty) {
        mesh->destroy_submesh(name);
    }

    S_DEBUG("Removed empty submeshes");

    mesh->vertex_data->done();

    S_DEBUG("Mesh loaded");
}

}
}
