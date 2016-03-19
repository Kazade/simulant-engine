#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <kazmath/vec3.h>
#include <kazmath/mat4.h>

#include <kazbase/logging.h>
#include <kazbase/unicode.h>

#include "../window.h"
#include "../stage.h"
#include "../mesh.h"
#include "../types.h"
#include "../light.h"
#include "../camera.h"
#include "../procedural/texture.h"
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

struct TextureInfo {
    Point3f u_axis;
    float u_offset;
    Point3f v_axis;
    float v_offset;

    uint32_t flags;
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

void Q2BSPLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Stage* stage = dynamic_cast<Stage*>(res_ptr);
    assert(stage && "You passed a Resource that is not a stage to the QBSP loader");

    std::map<std::string, TextureID> texture_lookup;
    kglt::TextureID checkerboard = stage->new_texture_from_file("kglt/materials/checkerboard.png");

    auto find_or_load_texture = [&](const std::string& texture_name) -> TextureID {
        if(texture_lookup.count(texture_name)) {
            return texture_lookup[texture_name];
        } else {
            TextureID new_texture_id;
            auto texture_filename = locate_texture(*stage->window->resource_locator.get(), texture_name);
            if(!texture_filename.empty()) {
                new_texture_id = stage->new_texture_from_file(texture_filename);
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

    /**
     *  Load the textures and generate materials
     */

    MeshID mid = stage->new_mesh();
    auto mesh = stage->mesh(mid);

    std::vector<MaterialID> materials;
    std::vector<std::pair<uint32_t, uint32_t>> texture_dimensions;
    std::unordered_map<MaterialID, SubMeshID> submeshes_by_material;

    materials.resize(textures.size());
    texture_dimensions.resize(textures.size());

    uint32_t tex_idx = 0;
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

        TextureID new_texture_id = find_or_load_texture(tex.texture_name);
        MaterialID new_material_id = stage->new_material_from_texture(new_texture_id);

        auto texture = stage->texture(new_texture_id);
        texture_dimensions[tex_idx].first = texture->width();
        texture_dimensions[tex_idx].second = texture->height();

        materials[tex_idx] = new_material_id;
        submeshes_by_material[new_material_id] = mesh->new_submesh_with_material(new_material_id);
        tex_idx++;
    }

    std::cout << "Num textures: " << materials.size() << std::endl;
    std::cout << "Num submeshes: " << mesh->submesh_count() << std::endl;

    for(Q2::Face& f: faces) {
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
        Q2::TextureInfo& tex = textures[f.texture_info];

        kmVec3 normal;
        kmVec3Fill(&normal, 0, 1, 0);

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
                    sm->index_data().index(index_lookup[tri_idx[j]]);
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

                float w = float(texture_dimensions[f.texture_info].first);
                float h = float(texture_dimensions[f.texture_info].second);

                mesh->shared_data().position(pos);
                mesh->shared_data().normal(normal);
                mesh->shared_data().diffuse(kglt::Colour::WHITE);
                mesh->shared_data().tex_coord0(u / w, v / h);
                mesh->shared_data().tex_coord1(u / w, v / h);
                mesh->shared_data().move_next();

                sm->index_data().index(mesh->shared_data().count() - 1);

                //Cache this new vertex in the lookup
                index_lookup[tri_idx[j]] = mesh->shared_data().count() - 1;
            }
        }
    }

    mesh->shared_data().done();
    mesh->each([&](SubMesh* submesh) {
        //Delete empty submeshes
        /*if(!submesh->index_data().count()) {
            mesh->delete_submesh(submesh->id());
            return;
        }*/
        submesh->index_data().done();
    });

    L_DEBUG(_u("Created an actor for mesh: {0}").format(mid));
    //Finally, create an actor from the world mesh
    stage->new_actor(mid);
}

}
}
