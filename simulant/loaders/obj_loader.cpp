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

#include "obj_loader.h"

#include "../mesh.h"
#include "../resource_manager.h"
#include "../shortcuts.h"

namespace smlt {
namespace loaders {

void parse_face(const unicode& input, int32_t& vertex_index, int32_t& tex_index, int32_t& normal_index) {
    /*
     *  Parses the following
     *  1//2
     *  1
     *  1/1
     *  1/1/1
     *  and outputs to the passed references. The index will equal -1 if it wasn't in the input
     */

    std::vector<unicode> parts = input.split("/");
    int32_t slash_count = input.count("/");

    if(slash_count == 0 || input.ends_with("//")) {
        tex_index = -1;
        normal_index = -1;
        vertex_index = parts[0].to_int();
    } else if(slash_count == 1) {
        normal_index = -1;
        vertex_index = parts[0].to_int();
        tex_index = parts[1].to_int();
    } else if(slash_count == 2) {
        if(input.contains("//")) {
            tex_index = -1;
        } else {
            tex_index = parts[1].to_int();
        }

        vertex_index = parts[0].to_int();
        normal_index = parts[parts.size() - 1].to_int();
    }

    //Handle the 1-based indexing
    if(vertex_index > -1) {
        vertex_index -= 1;
    }

    if(tex_index > -1) {
        tex_index -= 1;
    }

    if(normal_index > -1) {
        normal_index -= 1;
    }
}

void OBJLoader::into(Loadable &resource, const LoaderOptions &options) {
    Mesh* mesh = loadable_to<Mesh>(resource);

    //Split on newlines
    std::vector<unicode> lines = unicode(this->data_->str()).split("\n");

    std::vector<Vec3> vertices;
    std::vector<Vec2> tex_coords;
    std::vector<Vec3> normals;

    std::unordered_map<unicode, smlt::MaterialPtr> materials;
    SubMesh* sm = nullptr;

    unicode current_material;

    VertexSpecification spec;
    spec.position_attribute = VERTEX_ATTRIBUTE_3F;
    spec.texcoord0_attribute = VERTEX_ATTRIBUTE_2F;
    spec.normal_attribute = VERTEX_ATTRIBUTE_3F;
    spec.diffuse_attribute = VERTEX_ATTRIBUTE_4F;
    mesh->reset(spec);

    SubMesh* default_submesh = nullptr;

    bool has_materials = false;
    for(uint32_t l = 0; l < lines.size(); ++l) {
        unicode line = lines[l].strip();

        //Ignore comments
        if(line.starts_with("#")) {
            continue;
        }

        if(line.empty()) {
            continue;
        }

        std::vector<unicode> parts = line.split("", -1, false); //Split on whitespace
        if(parts[0] == "v") {
            if(parts.size() != 4) {
                throw std::runtime_error(_F("Found {0} components for vertex, expected 3").format(parts.size()));
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();
            float z = parts[3].to_float();

            vertices.push_back(Vec3(x, y, z));
        } else if(parts[0] == "vt") {
            if(parts.size() < 3) {
                throw std::runtime_error(_F("Found {0} components for texture coordinate, expected 2").format(parts.size() - 1));
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();

            tex_coords.push_back(Vec2(x, y));
        } else if(parts[0] == "vn") {
            if(parts.size() != 4) {
                throw std::runtime_error(_F("Found {0} components for vertex, expected 3").format(parts.size() - 1));
            }

            float x = parts[1].to_float();
            float y = parts[2].to_float();
            float z = parts[3].to_float();

            Vec3 n(x, y, z);
            kmVec3Normalize(&n, &n);
            normals.push_back(n);
        } else if(parts[0] == "f") {
            std::string smi;
            if(current_material.empty()) {
                if(!default_submesh) {
                    default_submesh = mesh->new_submesh("default");
                }
                smi = "default";
            } else {
                if(materials.count(current_material)) {
                    auto mat_id = materials.at(current_material)->id();
                    if(mesh->has_submesh(current_material.encode())) {
                        smi = current_material.encode();
                    } else {
                        mesh->new_submesh_with_material(current_material.encode(), mat_id);
                        smi = current_material.encode();
                    }
                } else {
                    L_WARN(_F("Ignoring non-existant material ({0}) while loading {1}").format(
                        current_material,
                        filename_
                    ));
                    if(sm) {
                        smi = sm->name(); // Just stick with the current submesh, don't change it
                    } else {
                        if(!default_submesh) {
                            default_submesh = mesh->new_submesh("default");
                        }
                        smi = "default";
                    }
                }
            }
            sm = mesh->submesh(smi);

            VertexData* vertex_data = sm->vertex_data.get();
            IndexData* index_data = sm->index_data.get();

            //Faces are a pain in the arse to parse
            parts = std::vector<unicode>(parts.begin() + 1, parts.end()); //Strip off the first bit

            /*
             * This loop looks weird because it builds a triangle fan
             * from the face indexes, as there could be more than 3. It goes
             * (0, 2, 1), (0, 3, 2) etc.
             */
            uint32_t first_index = 0;
            uint32_t i = 0;
            for(auto& part: parts) {
                int32_t v, tc, n;
                parse_face(part, v, tc, n);

                vertex_data->position(vertices[v]);

                if(tc != -1) {
                    vertex_data->tex_coord0(tex_coords[tc]);
                }

                if(n != -1) {
                    vertex_data->normal(normals[n]);
                }
                vertex_data->diffuse(smlt::Colour::WHITE);
                vertex_data->move_next();

                if(i == 2) {
                    first_index = vertex_data->count() - 3;
                }

                if(i >= 2) {
                    index_data->index(first_index);
                    index_data->index(vertex_data->count() - 1);
                    index_data->index(vertex_data->count() - 2);
                }

                ++i;
            }
        } else if(parts[0] == "newmtl") {
            auto material_name = parts[1];

            // Clone the default material
            materials[material_name] = mesh->resource_manager().clone_default_material().fetch();
            current_material = material_name;

            has_materials = true;
        } else if(parts[0] == "usemtl") {
            current_material = parts[1];
        } else if(parts[0] == "mtllib") {
            /*
             * If we find a mtllib command, we load the material file and insert its
             * lines into this position. This makes it so it's like the materials are embedded
             * in the current file which makes parsing the same as if they were!
             */

             // We need to re-get the filename because mtllib paths may have spaces and the existing 'parts'
             // variable would have split it to pieces.
             auto filename = line.split(" ", 1, false).back();
             filename = kfs::path::join(kfs::path::dir_name(filename_.encode()), filename.encode());

             try {
                filename = this->locator->locate_file(filename);
                auto material_lines = this->locator->read_file_lines(filename);
                lines.insert(lines.begin() + l + 1, material_lines.begin(), material_lines.end());
             } catch(ResourceMissingError& e) {
                 L_DEBUG(_F("mtllib {0} not found. Skipping.").format(filename));
             }
        } else if(parts[0] == "Ns") {
            auto mat = materials.at(current_material);
            mat->pass(0)->set_shininess(parts[1].to_float());
        } else if(parts[0] == "Ka") {
            auto mat = materials.at(current_material);

            float r = parts[1].to_float();
            float g = parts[2].to_float();
            float b = parts[3].to_float();

            mat->pass(0)->set_ambient(smlt::Colour(r, g, b, 1.0));
        } else if(parts[0] == "Kd") {
            auto mat = materials.at(current_material);

            float r = parts[1].to_float();
            float g = parts[2].to_float();
            float b = parts[3].to_float();

            mat->pass(0)->set_diffuse(smlt::Colour(r, g, b, 1.0));
        } else if(parts[0] == "Ks") {
            auto mat = materials.at(current_material);

            float r = parts[1].to_float();
            float g = parts[2].to_float();
            float b = parts[3].to_float();

            mat->pass(0)->set_specular(smlt::Colour(r, g, b, 1.0));
        } else if(parts[0] == "Ni") {

        } else if(parts[0] == "d") {
            // Dissolved == Transparency... apparently
        } else if(parts[0] == "illum") {

        } else if(parts[0] == "map_Kd") {
            auto texture_name = parts[1];
            auto mat = materials.at(current_material);

            std::vector<std::string> possible_locations;

            // Check relative texture file first
            possible_locations.push_back(
                kfs::path::join(
                    kfs::path::dir_name(filename_.encode()),
                    kfs::path::split(texture_name.encode()).second
                )
            );

            // Check potentially absolute file path
            possible_locations.push_back(texture_name.encode());

            bool found = false;
            for(auto& texture_file: possible_locations) {
                if(kfs::path::exists(texture_file)) {
                    auto tex_id = mesh->resource_manager().new_texture_from_file(texture_file);
                    mat->set_texture_unit_on_all_passes(0, tex_id);
                    found = true;
                    break;
                }
            }

            if(!found) {
                L_WARN(_F("Unable to locate texture {0}").format(texture_name.encode()));
            }
        }
    }


    if(normals.empty()) {
        mesh->each([&](const std::string& name, SubMesh* submesh) {
            VertexData* vertex_data = submesh->vertex_data.get();
            IndexData* index_data = submesh->index_data.get();

            // The mesh didn't have any normals, let's generate some!
            std::unordered_map<int, smlt::Vec3> index_to_normal;

            // Go through all the triangles, add the face normal to all the vertices
            for(uint16_t i = 0; i < index_data->count(); i+=3) {
                uint16_t idx1 = index_data->at(i);
                uint16_t idx2 = index_data->at(i+1);
                uint16_t idx3 = index_data->at(i+2);

                smlt::Vec3 v1, v2, v3;
                v1 = vertex_data->position_at<Vec3>(idx1);
                v2 = vertex_data->position_at<Vec3>(idx2);
                v3 = vertex_data->position_at<Vec3>(idx3);

                smlt::Vec3 normal = (v2 - v1).normalized().cross((v3 - v1).normalized()).normalized();

                index_to_normal[idx1] += normal;
                index_to_normal[idx2] += normal;
                index_to_normal[idx3] += normal;
            }

            // Now set the normal on the vertex data
            for(auto p: index_to_normal) {
                vertex_data->move_to(p.first);
                vertex_data->normal(p.second.normalized());
            }
        });
    }

    if(!has_materials) {
        //If the OBJ file has no materials, have a look around for textures in the same directory

        auto parts = kfs::path::split_ext(filename_.encode());

        std::vector<unicode> possible_diffuse_maps = {
            parts.first + ".jpg",
            parts.first + "_color.jpg",
            parts.first + "_diffuse.jpg",
            parts.first + ".png",
            parts.first + "_color.png",
            parts.first + "_diffuse.png",
            parts.first + ".dds",
            parts.first + "_color.dds",
            parts.first + "_diffuse.dds",
        };

        for(const unicode& p: possible_diffuse_maps) {
            if(kfs::path::exists(p.encode())) {
                //Create a material from it and apply it to the submesh
                MaterialID mat = mesh->resource_manager().new_material_from_texture(
                    mesh->resource_manager().new_texture_from_file(p.encode())
                );

                sm->set_material_id(mat);
                break;
            }
        }
    }

    mesh->shared_data->done();
    mesh->each([](const std::string&, SubMesh* submesh) {
        submesh->index_data->done();
    });
}

}
}
