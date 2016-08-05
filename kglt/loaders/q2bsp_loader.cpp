#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <kazmath/vec3.h>
#include <kazmath/mat4.h>

#include <kazbase/logging.h>
#include <kazbase/unicode.h>

#include "../sdl2_window.h"
#include "../stage.h"
#include "../mesh.h"
#include "../types.h"
#include "../light.h"
#include "../camera.h"
#include "../procedural/texture.h"
#include "../controllers/material/flowing.h"
#include "../controllers/material/warp.h"

#include "q2bsp_loader.h"

#include "kglt/shortcuts.h"

namespace kglt  {
namespace loaders {

namespace Q2 {

enum LumpType {
    ENTITIES = 0,
    PLANES,
    VERTICES,
    VISIBILITY,
    NODES,
    TEXTURE_INFO,
    FACES,
    LIGHTMAPS,
    LEAVES,
    LEAF_FACE_TABLE,
    LEAF_BRUSH_TABLE,
    EDGES,
    FACE_EDGE_TABLE,
    MODELS,
    BRUSHES,
    BRUSH_SIDES,
    POP,
    AREAS,
    AREA_PORTALS,
    MAX_LUMPS
};

typedef kmVec3 Point3f;

struct Point3s {
    int16_t x;
    int16_t y;
    int16_t z;
};

struct Edge {
    uint16_t a;
    uint16_t b;
};

enum SurfaceFlag {
    SURFACE_FLAG_NONE = 0x0,
    SURFACE_FLAG_LIGHT = 0x1,
    SURFACE_FLAG_SLICK = 0x2,
    SURFACE_FLAG_SKY = 0x4,
    SURFACE_FLAG_WARP = 0x8,
    SURFACE_FLAG_TRANS_33 = 0x10,
    SURFACE_FLAG_TRANS_66 = 0x20,
    SURFACE_FLAG_FLOWING = 0x40,
    SURFACE_FLAG_NO_DRAW = 0x80
};

struct TextureInfo {
    Point3f u_axis;
    float u_offset;
    Point3f v_axis;
    float v_offset;

    SurfaceFlag flags;
    uint32_t value;

    char texture_name[32];

    uint32_t next_tex_info;
};

struct Face {
    uint16_t plane;             // index of the plane the face is parallel to
    uint16_t plane_side;        // set if the normal is parallel to the plane normal
    uint32_t first_edge;        // index of the first edge (in the face edge array)
    uint16_t num_edges;         // number of consecutive edges (in the face edge array)
    uint16_t texture_info;      // index of the texture info structure
    uint8_t lightmap_syles[4]; // styles (bit flags) for the lightmaps
    uint32_t lightmap_offset;   // offset of the lightmap (in bytes) in the lightmap lump
};

struct Lump {
    uint32_t offset;
    uint32_t length;
};

struct Header {
    uint8_t magic[4];
    uint32_t version;

    Lump lumps[MAX_LUMPS];
};

}

typedef std::map<std::string, std::string> ActorProperties;

void parse_actors(const std::string& actor_string, std::vector<ActorProperties>& actors) {
    bool inside_actor = false;
    ActorProperties current;
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

Vec3 find_player_spawn_point(std::vector<ActorProperties>& actors) {
    for(ActorProperties p: actors) {
        std::cout << p["classname"] << std::endl;
        if(p["classname"] == "info_player_start") {
            Vec3 pos;
            std::istringstream origin(p["origin"]);
            origin >> pos.x >> pos.y >> pos.z;
            std::cout << "Setting start position to: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            return pos;
        }
    }
    Vec3 none;
    kmVec3Fill(&none, 0, 0, 0);

    return none;
}

void add_lights_to_scene(Stage* stage, const std::vector<ActorProperties>& actors) {
    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    for(ActorProperties props: actors) {
        if(props["classname"] == "light") {
            kmVec3 pos;
            std::istringstream origin(props["origin"]);
            origin >> pos.x >> pos.y >> pos.z;

            {
                auto new_light = stage->light(stage->new_light());
                new_light->set_absolute_position(pos.x, pos.y, pos.z);
                kmVec3Transform(&pos, &pos, &rotation);

                float range = 300; //Default in Q2
                if(container::contains(props, std::string("light"))) {
                    std::string tmp = props["light"];
                    std::istringstream value(tmp);
                    value >> range;
                }

                if(container::contains(props, std::string("_color"))) {
                    kglt::Colour diffuse;
                    std::istringstream value(props["_color"]);
                    value >> diffuse.r >> diffuse.g >> diffuse.b;
                    diffuse.a = 1.0;
                    new_light->set_diffuse(diffuse);
                }


                std::cout << "Creating light at: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
                new_light->set_attenuation_from_range(range);
            }
        }
    }
}

unicode locate_texture(ResourceLocator& locator, const unicode& filename) {
    std::vector<unicode> extensions = { ".wal", ".jpg", ".tga", ".jpeg", ".png" };
    for(auto& ext: extensions) {
        try {
            return locator.locate_file(filename + ext);
        } catch(IOError&) {
            continue;
        }
    }

    return "";
}

struct LightmapBuffer {
    static const uint32_t LIGHTMAP_SIZE = 18; //16 + 1 px border each side
    static const uint32_t LIGHTMAP_CHANNELS = 3;

    LightmapBuffer(uint32_t num_lightmaps) {
        horiz = ceil(sqrt(num_lightmaps)) + 1;
        vert = ceil(float(num_lightmaps) / float(horiz)) + 1;

        uint32_t buffer_size = (horiz * LIGHTMAP_SIZE) * (vert * LIGHTMAP_SIZE);
        buffer_size *= LIGHTMAP_CHANNELS;

        buffer.resize(buffer_size, 255);
    }

    void write_lightmap(uint32_t index, uint8_t* data, uint32_t width, uint32_t height) {
        uint32_t texture_width = LIGHTMAP_SIZE * LIGHTMAP_CHANNELS;
        uint32_t buffer_width = horiz * LIGHTMAP_SIZE * LIGHTMAP_CHANNELS;

        uint32_t target_x = index % horiz;
        uint32_t target_y = index / horiz;

        uint32_t texel_start = ((target_y * LIGHTMAP_SIZE) * buffer_width) + (target_x * texture_width);
        uint32_t texel = texel_start;

        for(int32_t y = -1; y < (int32_t) height + 1; ++y) {
            auto row_start = texel;
            for(int32_t x = -1; x < (int32_t) width + 1; ++x) {
                uint32_t src_x = (x < 0) ? 0 : (x >= (int32_t) width) ? width - 1 : x;
                uint32_t src_y = (y < 0) ? 0 : (y >= (int32_t) height) ? height - 1 : y;

                uint32_t source_idx = ((src_y * width) + src_x) * 3;

                // If we are within the bounds of the source lightmap, then
                // copy the data for this texel to the right place
                buffer.at(texel) = data[source_idx];
                buffer.at(texel + 1) = data[source_idx + 1];
                buffer.at(texel + 2) = data[source_idx + 2];
                // Move to the next texl
                texel += LIGHTMAP_CHANNELS;
            }

            texel = row_start + buffer_width;
        }
    }

    std::pair<float, float> transform_uv(int32_t index, float u, float v) const {
        float ret_u = u / float(horiz);
        float ret_v = v / float(vert);

        float x = float(index % horiz);
        float y = float(index / horiz);

        ret_u += (x * (1.0 / float(horiz)));
        ret_v += (y * (1.0 / float(vert)));

        return std::make_pair(ret_u, ret_v);
    }

    const uint32_t width_in_texels() const {
        return horiz * LIGHTMAP_SIZE;
    }

    const uint32_t height_in_texels() const {
        return vert * LIGHTMAP_SIZE;
    }

    uint32_t horiz = 0;
    uint32_t vert = 0;
    std::vector<uint8_t> buffer;
};

void Q2BSPLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Stage* stage = dynamic_cast<Stage*>(res_ptr);
    assert(stage && "You passed a Resource that is not a stage to the QBSP loader");

    std::map<std::string, TextureID> texture_lookup;
    TextureID checkerboard = stage->resources->new_texture_from_file(Texture::BuiltIns::CHECKERBOARD);
    TextureID lightmap_texture = stage->resources->new_texture(GARBAGE_COLLECT_NEVER);

    auto texture_info_visible = [](Q2::TextureInfo& info) -> bool {
        /* Don't draw invisible things */
        if((info.flags & Q2::SURFACE_FLAG_NO_DRAW) == Q2::SURFACE_FLAG_NO_DRAW) {
            return false;
        }

        /* Don't use sky faces (might change this... could create a skybox) */
        if((info.flags & Q2::SURFACE_FLAG_SKY) == Q2::SURFACE_FLAG_SKY) {
            return false;
        }

        return true;
    };

    auto find_or_load_texture = [&](const std::string& texture_name) -> TextureID {
        if(texture_lookup.count(texture_name)) {
            return texture_lookup[texture_name];
        } else {
            TextureID new_texture_id;
            auto texture_filename = locate_texture(*stage->window->resource_locator.get(), texture_name);
            if(!texture_filename.empty()) {
                new_texture_id = stage->resources->new_texture_from_file(texture_filename);
            } else {
                L_DEBUG(_u("Texture {0} was missing").format(texture_name));
                new_texture_id = checkerboard;
            }
            texture_lookup[texture_name] = new_texture_id;
            return new_texture_id;
        }
    };

    std::ifstream file(filename_.encode().c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the BSP file: " + filename_.encode());
    }

    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    std::vector<char> actor_buffer(header.lumps[Q2::LumpType::ENTITIES].length);
    file.seekg(header.lumps[Q2::LumpType::ENTITIES].offset);
    file.read(&actor_buffer[0], sizeof(char) * header.lumps[Q2::LumpType::ENTITIES].length);
    std::string actor_string(actor_buffer.begin(), actor_buffer.end());


    std::vector<ActorProperties> actors;
    parse_actors(actor_string, actors);
    Vec3 cam_pos = find_player_spawn_point(actors);
    stage->stash(cam_pos, "player_spawn");

    add_lights_to_scene(stage, actors);

    int32_t num_vertices = header.lumps[Q2::LumpType::VERTICES].length / sizeof(Q2::Point3f);
    std::vector<Q2::Point3f> vertices(num_vertices);
    file.seekg(header.lumps[Q2::LumpType::VERTICES].offset);
    file.read((char*)&vertices[0], sizeof(Q2::Point3f) * num_vertices);

    int32_t num_edges = header.lumps[Q2::LumpType::EDGES].length / sizeof(Q2::Edge);
    std::vector<Q2::Edge> edges(num_edges);
    file.seekg(header.lumps[Q2::LumpType::EDGES].offset);
    file.read((char*)&edges[0], sizeof(Q2::Edge) * num_edges);

    int32_t num_textures = header.lumps[Q2::LumpType::TEXTURE_INFO].length / sizeof(Q2::TextureInfo);
    std::vector<Q2::TextureInfo> textures(num_textures);
    file.seekg(header.lumps[Q2::LumpType::TEXTURE_INFO].offset);
    file.read((char*)&textures[0], sizeof(Q2::TextureInfo) * num_textures);

    //Read in the faces
    int32_t num_faces = header.lumps[Q2::LumpType::FACES].length / sizeof(Q2::Face);
    std::vector<Q2::Face> faces(num_faces);
    file.seekg(header.lumps[Q2::LumpType::FACES].offset);
    file.read((char*)&faces[0], sizeof(Q2::Face) * num_faces);

    int32_t num_face_edges = header.lumps[Q2::LumpType::FACE_EDGE_TABLE].length / sizeof(int32_t);
    std::vector<int32_t> face_edges(num_face_edges);
    file.seekg(header.lumps[Q2::LumpType::FACE_EDGE_TABLE].offset);
    file.read((char*)&face_edges[0], sizeof(int32_t) * num_face_edges);

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

    int32_t lightmap_data_size = header.lumps[Q2::LumpType::LIGHTMAPS].length;
    std::vector<uint8_t> lightmap_data(lightmap_data_size);
    file.seekg(header.lumps[Q2::LumpType::LIGHTMAPS].offset);
    file.read((char*)&lightmap_data[0], lightmap_data_size);

    /**
     *  Load the textures and generate materials
     */

    MeshID mid = stage->resources->new_mesh_with_alias(
        "world_geometry",
        VertexSpecification::DEFAULT,
        GARBAGE_COLLECT_NEVER
    );
    auto mesh = stage->resources->mesh(mid);

    std::vector<MaterialID> materials;
    std::vector<std::pair<uint32_t, uint32_t>> texture_dimensions;
    std::unordered_map<MaterialID, SubMeshID> submeshes_by_material;

    materials.resize(textures.size());
    texture_dimensions.resize(textures.size());

    uint32_t tex_idx = 0;
    for(Q2::TextureInfo& tex: textures) {
        if(!texture_info_visible(tex)) {
            ++tex_idx;
            continue;
        }

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

        TextureID new_texture_id = find_or_load_texture(tex.texture_name);

        MaterialID new_material_id;

        bool uses_lightmap = !(tex.flags & (Q2::SURFACE_FLAG_SKY | Q2::SURFACE_FLAG_WARP));
        if(uses_lightmap) {
            new_material_id = stage->resources->new_material_from_file(Material::BuiltIns::TEXTURE_WITH_LIGHTMAP);
        } else {
            new_material_id = stage->resources->new_material_from_file(Material::BuiltIns::TEXTURE_ONLY);
        }

        {
            auto mat = stage->resources->material(new_material_id);
            mat->pass(0)->set_texture_unit(0, new_texture_id);

            if(uses_lightmap) {
                mat->pass(0)->set_texture_unit(1, lightmap_texture);
            }

            if(tex.flags & Q2::SURFACE_FLAG_FLOWING) {
                mat->new_controller<controllers::material::Flowing>();
            } else if(tex.flags & Q2::SURFACE_FLAG_WARP) {
                mat->new_controller<controllers::material::Warp>();
            }

            if(tex.flags & Q2::SURFACE_FLAG_TRANS_33) {
                mat->pass(0)->set_diffuse(kglt::Colour(1.0, 1.0, 1.0, 0.33));
                mat->pass(0)->set_blending(BLEND_ALPHA);
            }

            if(tex.flags & Q2::SURFACE_FLAG_TRANS_66) {
                mat->pass(0)->set_diffuse(kglt::Colour(1.0, 1.0, 1.0, 0.66));
                mat->pass(0)->set_blending(BLEND_ALPHA);
            }
        }

        auto texture = stage->resources->texture(new_texture_id);
        texture_dimensions[tex_idx].first = texture->width();
        texture_dimensions[tex_idx].second = texture->height();

        materials[tex_idx] = new_material_id;
        submeshes_by_material[new_material_id] = mesh->new_submesh_with_material(new_material_id);
        tex_idx++;
    }

    std::cout << "Num textures: " << texture_lookup.size() << std::endl;
    std::cout << "Num submeshes: " << mesh->submesh_count() << std::endl;


    typedef uint32_t Offset;
    typedef uint32_t FaceIndex;

    struct LightmapInfo {
        Offset offset;
        uint32_t width;
        uint32_t height;
        short min_u;
        short min_v;
    };

    struct StagedLightmapCoord {
        FaceIndex face_index;
        int32_t cursor_position;
        float u;
        float v;
    };

    std::vector<StagedLightmapCoord> lightmap_coords_to_process;
    std::map<FaceIndex, LightmapInfo> lightmaps;

    int32_t face_index = -1;
    for(Q2::Face& f: faces) {
        Q2::TextureInfo& tex = textures[f.texture_info];

        if(!texture_info_visible(tex)) {
            continue;
        }

        ++face_index;

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

        auto material_id = materials[f.texture_info];
        SubMesh* sm = mesh->submesh(submeshes_by_material[material_id]);

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

        short min_u = 10000, max_u = -10000;
        short min_v = 10000, max_v = -10000;

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
                if(container::contains(index_lookup, tri_idx[j])) {
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

                float w = float(texture_dimensions[f.texture_info].first);
                float h = float(texture_dimensions[f.texture_info].second);

                mesh->shared_data->position(pos);
                mesh->shared_data->normal(normal);
                mesh->shared_data->diffuse(kglt::Colour::WHITE);
                mesh->shared_data->tex_coord0(u / w, v / h);
                mesh->shared_data->tex_coord1(u / w, v / h);

                StagedLightmapCoord coord;
                coord.cursor_position = mesh->shared_data->cursor_position();
                coord.face_index = face_index;
                coord.u = u;
                coord.v = v;

                lightmap_coords_to_process.push_back(coord);

                mesh->shared_data->move_next();

                sm->index_data->index(mesh->shared_data->count() - 1);

                //Cache this new vertex in the lookup
                index_lookup[tri_idx[j]] = mesh->shared_data->count() - 1;
            }
        }

        uint32_t lightmap_width  = ceil(max_u / 16) - floor(min_u / 16) + 1;
        uint32_t lightmap_height = ceil(max_v / 16) - floor(min_v / 16) + 1;

        lightmap_width = std::min(lightmap_width, (uint32_t)16);
        lightmap_height = std::min(lightmap_height, (uint32_t)16);

        if(!lightmaps.count(face_index)) {
            lightmaps[face_index].offset = f.lightmap_offset;
            lightmaps[face_index].width = lightmap_width;
            lightmaps[face_index].height = lightmap_height;
            lightmaps[face_index].min_u = min_u;
            lightmaps[face_index].min_v = min_v;
        }
    }

    std::cout << "Num lightmaps: " << lightmaps.size() << std::endl;
    LightmapBuffer lightmap_buffer(lightmaps.size());

    for(auto& p: lightmaps) {
        auto face_index = p.first;
        auto& lightmap = p.second;

        lightmap_buffer.write_lightmap(
            face_index,
            &lightmap_data[0] + lightmap.offset,
            lightmap.width,
            lightmap.height
        );
    }

    auto pos = mesh->shared_data->cursor_position();
    for(StagedLightmapCoord& staged_coord: lightmap_coords_to_process) {
        auto& lightmap = lightmaps[staged_coord.face_index];
        float u = staged_coord.u;
        float v = staged_coord.v;

        u = u - float(lightmap.min_u) + 8.0f;
        u /= (float(lightmap.width) * 16.0f);

        v = v - float(lightmap.min_v) + 8.0f;
        v /= (float(lightmap.height) * 16.0f);

        // These two lines are hacky, and in the wrong place. Basically we need to scale by width / LIGHTMAP_SIZE
        // because our lightmap sections in the buffer have extra space (the max size for a lightmap)
        u *= float(lightmap.width) / LightmapBuffer::LIGHTMAP_SIZE;
        v *= float(lightmap.height) / LightmapBuffer::LIGHTMAP_SIZE;

        mesh->shared_data->move_to(staged_coord.cursor_position);
        auto lightmap_coords = lightmap_buffer.transform_uv(staged_coord.face_index, u, v);
        mesh->shared_data->tex_coord1(lightmap_coords.first, lightmap_coords.second);
    }
    mesh->shared_data->move_to(pos);

    /* Now upload the lightmap texture */
    {
        auto lightmap = stage->resources->texture(lightmap_texture);
        lightmap->resize(lightmap_buffer.width_in_texels(), lightmap_buffer.height_in_texels());
        lightmap->set_bpp(24); // RGB only, no alpha
        lightmap->data().assign(lightmap_buffer.buffer.begin(), lightmap_buffer.buffer.end());
        lightmap->upload(MIPMAP_GENERATE_NONE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_LINEAR, false);
    }

    mesh->stash(lightmap_texture, "lightmap_texture_id");
    mesh->shared_data->done();
    mesh->each([&](SubMesh* submesh) {
        //Delete empty submeshes
        /*if(!submesh->index_data->count()) {
            mesh->delete_submesh(submesh->id());
            return;
        }*/
        submesh->index_data->done();
    });

    //Finally, create an actor from the world mesh
    stage->new_actor_with_mesh(mid);

    // Now the mesh has been attached, it can be collected
    mesh->enable_gc();
    L_DEBUG(_u("Created a geom for mesh: {0}").format(mid));
}

}
}
