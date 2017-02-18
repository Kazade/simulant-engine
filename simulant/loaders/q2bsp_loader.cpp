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

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../deps/kazlog/kazlog.h"

#include "../sdl2_window.h"
#include "../stage.h"
#include "../mesh.h"
#include "../types.h"
#include "../nodes/light.h"
#include "../camera.h"
#include "../procedural/texture.h"
#include "../controllers/material/flowing.h"
#include "../controllers/material/warp.h"
#include "../utils/rect_pack.h"

#include "q2bsp_loader.h"

#include "simulant/shortcuts.h"

namespace smlt  {
namespace loaders {

const int LIGHTMAP_DIMENSION = 1024; // Should be enough for anybody...

void parse_actors(const std::string& actor_string, Q2EntityList& actors) {
    bool inside_actor = false;
    Q2Entity current;
    unicode key, value;
    bool inside_key = false, inside_value = false, key_done_for_this_line = false;
    for(char c: actor_string) {
        if(c == '{' && !inside_actor) {
            inside_actor = true;
            current.clear();
        }
        else if(c == '}' && inside_actor) {
            assert(inside_actor);
            inside_actor = false;
            actors.push_back(current);
        }
        else if(c == '\n' || c == '\r') {
            key_done_for_this_line = false;
            if(!key.empty() && !value.empty()) {
                key = key.strip();
                value = value.strip();

                current[key.encode()] = value.encode();
                std::cout << "Storing {" << key << " : " << value << "}" << std::endl;
            }
            key = "";
            value = "";
        }
        else if (c == '"') {
            if(!inside_key && !inside_value) {
                if(!key_done_for_this_line) {
                    inside_key = true;
                } else {
                    inside_value = true;
                }

            }
            else if(inside_key) {
                inside_key = false;
                key_done_for_this_line = true;
            } else {
                inside_value = false;
            }
        }
        else {
            if(inside_key) {
                key.push_back(c);
            } else {
                value.push_back(c);
            }

        }
    }

}

unicode locate_texture(ResourceLocator& locator, const unicode& filename) {
    std::vector<unicode> extensions = { ".wal", ".jpg", ".tga", ".jpeg", ".png" };
    for(auto& ext: extensions) {
        try {
            return locator.locate_file(filename + ext);
        } catch(std::runtime_error&) {
            continue;
        }
    }

    return Texture::BuiltIns::CHECKERBOARD;
}

template<typename T>
uint32_t read_lump(std::istream& file, const Q2::Header& header, Q2::LumpType type, std::vector<T>& lumpout) {
    uint32_t count = header.lumps[type].length / sizeof(T);
    lumpout.resize(count);
    file.seekg((std::istream::pos_type) header.lumps[type].offset);
    file.read((char*)&lumpout[0], (int) sizeof(T) * count);
    return count;
}

bool has_bitflag(uint32_t val, uint32_t flag) {
    return (val & flag) == flag;
}

void Q2BSPLoader::generate_materials(
    ResourceManager* assets,
    const std::vector<Q2::TextureInfo>& texture_infos,
    std::vector<MaterialID>& materials,
    std::vector<Q2::TexDimension>& dimensions,
    TextureID lightmap_texture) {

    /* Given a list of texture infos, this generates counterpart materials */

    std::map<unicode, TextureID> textures;

    materials.clear();
    for(auto& info: texture_infos) {
        bool is_invisible = has_bitflag(info.flags, Q2::SURFACE_FLAG_NO_DRAW);
        bool uses_lightmap = !(has_bitflag(info.flags, Q2::SURFACE_FLAG_SKY) || has_bitflag(info.flags, Q2::SURFACE_FLAG_WARP));

        if(is_invisible) {
            materials.push_back(MaterialID()); // Just push a null material for invisible surfaces
            dimensions.push_back(Q2::TexDimension(0, 0));
            continue;
        }

        unicode texture_name = info.texture_name;

        // Only load each texture once
        TextureID tex_id;
        if(!textures.count(texture_name)) {
            unicode full_path = locate_texture(locator, texture_name);
            tex_id = assets->new_texture_from_file(full_path);
            textures[texture_name] = tex_id;
        } else {
            tex_id = textures.at(texture_name);
        }

        // Load the correct material depending on surface flags
        auto material_id = assets->new_material_from_file(
            (uses_lightmap) ? Material::BuiltIns::TEXTURE_WITH_LIGHTMAP : Material::BuiltIns::TEXTURE_ONLY
        );

        auto mat = material_id.fetch();

        mat->first_pass()->set_texture_unit(0, tex_id);
        if(uses_lightmap) {
            // Set the second texture unit to the lightmap texture if necessary
            mat->first_pass()->set_texture_unit(1, lightmap_texture);
        }

        // Important, we fetched but we don't want it collected yet
        assets->mark_material_as_uncollected(material_id);

        auto tex = tex_id.fetch();
        materials.push_back(material_id);
        dimensions.push_back(Q2::TexDimension(tex->width(), tex->height()));
    }
}

struct Lightmap {
    std::vector<uint8_t> data;
    uint32_t width;
    uint32_t height;
};

struct FaceUVLimits {
    Vec2 min;
    Vec2 max;
};

std::vector<Lightmap> extract_lightmaps(const std::vector<uint8_t> lightmap_data, const std::vector<Q2::Face>& faces, const std::vector<FaceUVLimits>& uv_limits) {
    std::vector<Lightmap> result;

    assert(faces.size() == uv_limits.size());

    for(uint32_t i = 0; i < faces.size(); ++i) {
        auto& face = faces[i];
        auto& uv_limit = uv_limits[i];

        Lightmap lmap;
        lmap.width = std::ceil(uv_limit.max.x / 16) - floor(uv_limit.min.x / 16) + 1;
        lmap.height = std::ceil(uv_limit.max.y / 16) - floor(uv_limit.min.y / 16) + 1;

        auto data_size = lmap.width * lmap.height * 3;
        lmap.data.assign(&lightmap_data[face.lightmap_offset], &lightmap_data[face.lightmap_offset + data_size]);
        result.push_back(lmap);
    }

    return result;
}

struct LightmapLocation {
    uint16_t x;
    uint16_t y;

    LightmapLocation() = default;
    LightmapLocation(uint16_t x, uint16_t y):
        x(x), y(y) {}
};

std::vector<LightmapLocation> pack_lightmaps(const std::vector<Lightmap>& lightmaps, smlt::TexturePtr output_texture) {
    stbrp_context context;
    std::vector<stbrp_node> temp_storage(LIGHTMAP_DIMENSION * 1.5);
    stbrp_init_target(&context, LIGHTMAP_DIMENSION, LIGHTMAP_DIMENSION, &temp_storage[0], temp_storage.size());

    std::vector<stbrp_rect> rects(lightmaps.size());
    for(uint32_t i = 0; i < lightmaps.size(); ++i) {
        rects[i].id = i;
        rects[i].w = lightmaps[i].width;
        rects[i].h = lightmaps[i].height;
    }

    // Pack the rectangles
    stbrp_pack_rects(&context, &rects[0], rects.size());

    // Finally generate the texture!
    output_texture->resize(LIGHTMAP_DIMENSION, LIGHTMAP_DIMENSION);
    output_texture->set_bpp(24);

    std::vector<LightmapLocation> locations(lightmaps.size());

    bool logged = false;
    for(uint32_t i = 0; i < lightmaps.size(); ++i) {
        auto& rect = rects[i];

        if(!rect.was_packed && !logged) {
            L_DEBUG("Ran out of space packing lightmaps!");
            logged = true;
        }

        uint32_t src_x = 0, src_y = 0;
        for(uint32_t y = rect.y; y < rect.y + rect.h; ++y) {
            for(uint32_t x = rect.x; x < rect.x + rect.w; ++x) {
                uint32_t idx = (y * LIGHTMAP_DIMENSION * 3) + (x * 3);
                uint32_t src_idx = (src_y * rect.w * 3) + (src_x * 3);

                output_texture->data()[idx] = lightmaps[i].data[src_idx];
                output_texture->data()[idx + 1] = lightmaps[i].data[src_idx + 1];
                output_texture->data()[idx + 2] = lightmaps[i].data[src_idx + 2];

                src_x++;
            }
            src_y++;
            src_x = 0;
        }

        locations[i] = LightmapLocation(rect.x, rect.y);
    }

    output_texture->save_to_file("/tmp/lightmap.tga");
    output_texture->upload(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_LINEAR);
    return locations;
}

void Q2BSPLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Mesh* mesh = dynamic_cast<Mesh*>(res_ptr);

    // Make sure the passed mesh is empty and using the default vertex spec
    mesh->reset(smlt::VertexSpecification::DEFAULT);

    auto assets = &mesh->resource_manager();
    auto& locator = assets->window->resource_locator;

    assert(mesh && "You passed a Resource that is not a mesh to the QBSP loader");

    auto& file = *this->data_;

    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    std::vector<char> actor_buffer;
    read_lump(file, header, Q2::LumpType::ENTITIES, actor_buffer);
    std::string actor_string(actor_buffer.begin(), actor_buffer.end());

    Q2EntityList entities;
    parse_actors(actor_string, entities);
    mesh->data->stash(entities, "entities");

    std::vector<Q2::Point3f> vertices;
    std::vector<Q2::Edge> edges;
    std::vector<Q2::TextureInfo> textures;
    std::vector<Q2::Face> faces;
    std::vector<uint32_t> face_edges;
    std::vector<uint8_t> lightmap_data;

    auto num_vertices = read_lump(file, header, Q2::LumpType::VERTICES, vertices);
    auto num_edges = read_lump(file, header, Q2::LumpType::EDGES, edges);
    auto num_textures = read_lump(file, header, Q2::LumpType::TEXTURE_INFO, textures);
    auto num_faces = read_lump(file, header, Q2::LumpType::FACES, faces);
    auto num_face_edges = read_lump(file, header, Q2::LumpType::FACE_EDGE_TABLE, face_edges);
    auto lightmap_length = read_lump(file, header, Q2::LumpType::LIGHTMAPS, lightmap_data);


    std::for_each(vertices.begin(), vertices.end(), [&](Q2::Point3f& vert) {
        // Dirty casts, but it should work...
        kmVec3Transform((kmVec3*)&vert, (kmVec3*)&vert, &rotation);
    });

    /* Lightmaps are a bunch of fun! There is one lightmap per-face, and that might be up to 16x16 - but
     * can be smaller. Each face has the byte offset to the lightmap in the file, and the width and height of
     * the lightmap is defined by some weird calculation. So you can't just read all the lightmaps without first
     * reading the faces. So we read the lump into memory, then while we process the faces we pack the lightmap into
     * a texture which is sqrt(num_faces) * 16 wide and tall and then manipulate the texture coords appropriately.
     */


    /**
     *  Load the textures and generate materials
     */

    std::vector<MaterialID> materials;    
    std::vector<Q2::TexDimension> dimensions;

    TextureID lightmap_texture = assets->new_texture();

    generate_materials(assets, textures, materials, dimensions, lightmap_texture);

    /* Transform U/V coordinates appropriately */
    for(Q2::TextureInfo& tex: textures) {
        kmVec3 u_axis, v_axis;
        kmVec3Fill(&u_axis, tex.u_axis.x, tex.u_axis.y, tex.u_axis.z);
        kmVec3Fill(&v_axis, tex.v_axis.x, tex.v_axis.y, tex.v_axis.z);
        kmVec3Transform(&u_axis, &u_axis, &rotation);
        kmVec3Transform(&v_axis, &v_axis, &rotation);
        tex.u_axis.x = u_axis.x;
        tex.u_axis.y = u_axis.y;
        tex.u_axis.z = u_axis.z;

        tex.v_axis.x = v_axis.x;
        tex.v_axis.y = v_axis.y;
        tex.v_axis.z = v_axis.z;
    }

    std::unordered_map<MaterialID, SubMesh*> submeshes_by_material;
    uint32_t i;
    for(auto& material: materials) {
        submeshes_by_material[material] = mesh->new_submesh_with_material(_F("{0}").format(i++), material);
    }

    std::cout << "Num materials: " << materials.size() << std::endl;
    std::cout << "Num submeshes: " << mesh->submesh_count() << std::endl;

    std::vector<FaceUVLimits> uv_limits;

    std::vector<std::set<uint32_t>> face_indexes(faces.size());

    int32_t face_id = -1;
    for(Q2::Face& f: faces) {
        FaceUVLimits uv_limit;

        auto& tex = textures[f.texture_info];
        auto material_id = materials.at(f.texture_info);
        if(!material_id) {
            // Must be an invisible surface
            uv_limits.push_back(uv_limit);
            continue;
        }

        ++face_id;

        std::vector<uint32_t> indexes;
        for(uint32_t i = f.first_edge; i < f.first_edge + f.num_edges; ++i) {
            int32_t edge_idx = face_edges[i];
            if(edge_idx > 0) {
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.a);
                //indexes.push_back(e.b);
            } else {
                edge_idx = -edge_idx;
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.b);
                //indexes.push_back(e.a);
            }
        }

        SubMesh* sm = submeshes_by_material.at(material_id);

        /*
         *  A unique vertex is defined by a combination of the position ID and the
         *  texture_info index (because texture coordinates depend on both and some
         *  some vertices must be duplicated.
         *
         *  Here we store a mapping so that we don't create duplicate vertices if we don't need to!
         */

        /*
         * Build the triangles for this "face"
         */

        std::map<uint32_t, uint32_t> index_lookup;
        kmVec3 normal;
        kmVec3Fill(&normal, 0, 1, 0);

        float min_u = std::numeric_limits<float>::max();
        float max_u = std::numeric_limits<float>::lowest();
        float min_v = std::numeric_limits<float>::max();
        float max_v = std::numeric_limits<float>::lowest();

        for(int16_t i = 1; i < (int16_t) indexes.size() - 1; ++i) {
            uint32_t tri_idx[] = {
                indexes[0],
                indexes[i+1],
                indexes[i]
            };

            //Calculate the surface normal for this triangle
            kmVec3 normal;
            kmVec3 vec1, vec2;
            kmVec3& v1 = vertices[tri_idx[0]];
            kmVec3& v2 = vertices[tri_idx[1]];
            kmVec3& v3 = vertices[tri_idx[2]];

            kmVec3Subtract(&vec1, &v2, &v1);
            kmVec3Subtract(&vec2, &v3, &v1);
            kmVec3Cross(&normal, &vec1, &vec2);
            kmVec3Normalize(&normal, &normal);

            for(uint8_t j = 0; j < 3; ++j) {
                if(index_lookup.count(tri_idx[j])) {
                    //We've already processed this vertex
                    sm->index_data->index(index_lookup[tri_idx[j]]);
                    continue;
                }


                kmVec3& pos = vertices[tri_idx[j]];

                //We haven't done this before so calculate the vertex
                float u = pos.x * tex.u_axis.x
                        + pos.y * tex.u_axis.y
                        + pos.z * tex.u_axis.z + tex.u_offset;

                float v = pos.x * tex.v_axis.x
                        + pos.y * tex.v_axis.y
                        + pos.z * tex.v_axis.z + tex.v_offset;

                if(u < min_u) min_u = u;
                if(u > max_u) max_u = u;
                if(v < min_v) min_v = v;
                if(v > max_v) max_v = v;

                float w = float(dimensions[f.texture_info].width);
                float h = float(dimensions[f.texture_info].height);

                mesh->shared_data->position(pos);
                mesh->shared_data->normal(normal);
                mesh->shared_data->diffuse(smlt::Colour::WHITE);
                mesh->shared_data->tex_coord0(u / w, v / h);
                mesh->shared_data->tex_coord1(u / w, v / h);
                mesh->shared_data->move_next();

                sm->index_data->index(mesh->shared_data->count() - 1);

                //Cache this new vertex in the lookup
                index_lookup[tri_idx[j]] = mesh->shared_data->count() - 1;

                // Record this vertex against this face so we can manipulate the lightmap coordinates
                // after packing
                face_indexes[face_id].insert(mesh->shared_data->count() - 1);
            }
        }

        uv_limit.min = Vec2(min_u, min_v);
        uv_limit.max = Vec2(max_u, max_v);
        uv_limits.push_back(uv_limit);
    }

    auto lightmaps = extract_lightmaps(lightmap_data, faces, uv_limits);
    auto locations = pack_lightmaps(lightmaps, lightmap_texture.fetch());

    for(uint32_t i = 0; i < locations.size(); ++i) {
        for(auto idx: face_indexes[i]) {

            float x_offset = float(locations[i].x) / float(LIGHTMAP_DIMENSION);
            float y_offset = float(locations[i].y) / float(LIGHTMAP_DIMENSION);

            mesh->shared_data->move_to(idx);
            auto t = mesh->shared_data->texcoord1_at<Vec2>(idx);

            t.x *= float(lightmaps[i].width) / float(LIGHTMAP_DIMENSION);
            t.y *= float(lightmaps[i].height) / float(LIGHTMAP_DIMENSION);

            mesh->shared_data->tex_coord1(t + Vec2(x_offset, y_offset));
        }
    }

    mesh->shared_data->done();
    mesh->each([&](const std::string& name, SubMesh* submesh) {
        //Delete empty submeshes
        /*if(!submesh->index_data->count()) {
            mesh->delete_submesh(name);
            return;
        }*/
        submesh->index_data->done();
    });


    //FIXME: mark mesh as uncollected
}

}
}
