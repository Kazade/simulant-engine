//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//
#include <string>

#define TINYOBJLOADER_IMPLEMENTATION
#include "./deps/tiny_obj_loader.h"

#include "obj_loader.h"

#include "../meshes/mesh.h"
#include "../asset_manager.h"
#include "../shortcuts.h"
#include "../resource_locator.h"
#include "../utils/string.h"

namespace smlt {
namespace loaders {

class SimulantMaterialReader : public tinyobj::MaterialReader {
public:
    SimulantMaterialReader(ResourceLocator* locator, const unicode& obj_filename):
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
        } catch(ResourceMissingError& e) {
            L_DEBUG(_F("mtllib {0} not found. Skipping.").format(filename));
        }

        return true;
    }

private:
    ResourceLocator* locator_ = nullptr;
    unicode obj_filename_;
};

void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);

    L_DEBUG(_F("Loading mesh from {0}").format(filename_));

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    SimulantMaterialReader reader(locator.get(), filename_);
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, data_.get(), &reader);

    if(!ret) {
        L_ERROR(_F("Unable to load .obj file {0}").format(filename_));
        L_ERROR(_F("Error was: {0}").format(err));
        return;
    }

    if(!warn.empty()) {
        L_WARN(warn);
    }

    L_DEBUG(_F("Mesh has {0} shapes and {1} materials").format(shapes.size(), materials.size()));

    VertexSpecification spec(VERTEX_ATTRIBUTE_3F);
    if(!attrib.normals.empty()) {
        spec.normal_attribute = VERTEX_ATTRIBUTE_3F;
    }

    if(!attrib.texcoords.empty()) {
        spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    }

    mesh->reset(spec);  // Make sure we're empty before we begin

    std::unordered_map<std::string, MaterialPtr> final_materials;
    std::unordered_map<uint32_t, SubMeshPtr> material_submeshes;
    std::unordered_map<std::string, TexturePtr> loaded_textures;

    auto index_type = (attrib.vertices.size() / 3 >= std::numeric_limits<uint16_t>::max()) ?
        INDEX_TYPE_32_BIT : INDEX_TYPE_16_BIT;

    // First, load the materials, and create submeshes
    uint32_t i = 0;
    for(auto& material: materials) {
        MaterialPtr new_mat = mesh->resource_manager().clone_default_material().fetch();
        MaterialPass::ptr pass = new_mat->first_pass();

        auto alpha = (material.dissolve) ? 1.0f : 0.0f;
        pass->set_diffuse(smlt::Colour(material.diffuse[0], material.diffuse[1], material.diffuse[2], alpha));
        pass->set_ambient(smlt::Colour(material.ambient[0], material.ambient[1], material.ambient[2], alpha));
        pass->set_specular(smlt::Colour(material.specular[0], material.specular[1], material.specular[2], alpha));
        pass->set_shininess(material.shininess);

        /* Apply the diffuse texture (if any) */
        if(!material.diffuse_texname.empty()) {
            auto it = loaded_textures.find(material.diffuse_texname);
            if(it != loaded_textures.end()) {
                pass->set_texture_unit(0, it->second->id());
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
                        auto tex_id = mesh->resource_manager().new_texture_from_file(texture_file);
                        pass->set_texture_unit(0, tex_id);
                        loaded_textures.insert(std::make_pair(material.diffuse_texname, tex_id.fetch()));
                        found = true;
                        break;
                    }
                }

                if(!found) {
                    L_WARN(_F("Unable to locate texture {0}").format(material.diffuse_texname));
                }
            }
        }

        final_materials.insert(std::make_pair(material.name, new_mat));

        auto submesh = mesh->new_submesh(material.name, MESH_ARRANGEMENT_TRIANGLES, index_type);
        submesh->set_material_id(new_mat->id());

        material_submeshes.insert(std::make_pair(i++, submesh));
    }

    typedef std::tuple<int, int, int> VertexKey;

    std::unordered_map<VertexKey, uint32_t> shared_vertices;

    float default_tc [] = {0.0f, 0.0f};
    float default_n [] = {0.0f, 0.0f, 1.0f};

    for(auto& shape: shapes) {
        L_DEBUG(_F("Converting shape {0}").format(shape.name));

        uint32_t offset = 0;
        for(uint32_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            uint8_t num_verts = shape.mesh.num_face_vertices[f];
            assert(num_verts == 3 && "Only triangles supported");

            auto submeshptr = material_submeshes.at(shape.mesh.material_ids[f]);

            for(auto i = 0; i < num_verts; ++i) {
                auto index = shape.mesh.indices[offset + i];
                auto key = std::make_tuple(
                    index.vertex_index, index.normal_index, index.texcoord_index
                );

                auto it = shared_vertices.find(key);
                if(it == shared_vertices.end()) {
                    float* pos = &attrib.vertices[3 * index.vertex_index];
                    float* tc = (index.texcoord_index == -1) ?
                        &default_tc[0] : &attrib.texcoords[2 * index.texcoord_index];
                    float* n = (index.normal_index == -1) ?
                        &default_n[0] : &attrib.normals[3 * index.normal_index];

                    mesh->vertex_data->position(pos[0], pos[1], pos[2]);
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
        }

    }

    L_DEBUG("Mesh loaded");
}

}
}
