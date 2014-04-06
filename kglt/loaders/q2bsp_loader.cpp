#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string/trim.hpp>
#include <kazmath/vec3.h>
#include <kazmath/mat4.h>

#include <kazbase/logging.h>
#include <kazbase/unicode.h>

#include "../window.h"
#include "../stage.h"
#include "../scene.h"
#include "../mesh.h"
#include "../types.h"
#include "../light.h"
#include "../camera.h"
#include "../procedural/texture.h"
#include <kazbase/string.h>
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

struct Point3f {
    float x;
    float y;
    float z;
};

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
    std::string key, value;
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
                boost::algorithm::trim(key);
                boost::algorithm::trim(value);

                current[key] = value;
                std::cout << "Storing {" << key << " : " << value << "}" << std::endl;
            }
            key.clear();
            value.clear();
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

kmVec3 find_player_spawn_point(std::vector<ActorProperties>& actors) {
    for(ActorProperties p: actors) {
        std::cout << p["classname"] << std::endl;
        if(p["classname"] == "info_player_start") {
            kmVec3 pos;
            std::istringstream origin(p["origin"]);
            origin >> pos.x >> pos.y >> pos.z;
            std::cout << "Setting start position to: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            return pos;
        }
    }
    kmVec3 none;
    kmVec3Fill(&none, 0, 0, 0);

    return none;
}

void add_lights_to_scene(Scene& scene, const std::vector<ActorProperties>& actors) {
    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    for(ActorProperties props: actors) {
        if(props["classname"] == "light") {
            kmVec3 pos;
            std::istringstream origin(props["origin"]);
            origin >> pos.x >> pos.y >> pos.z;

            {
                auto new_light = scene.stage()->light(scene.stage()->new_light());
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

void Q2BSPLoader::into(Loadable& resource, const LoaderOptions &options) {
    Loadable* res_ptr = &resource;
    Scene* scene = dynamic_cast<Scene*>(res_ptr);
    assert(scene && "You passed a Resource that is not a scene to the Scene loader");

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
    kmVec3 cam_pos = find_player_spawn_point(actors);
    kmVec3Transform(&cam_pos, &cam_pos, &rotation);
    scene->stage()->camera()->set_absolute_position(cam_pos);

    add_lights_to_scene(*scene, actors);

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

    std::vector<kmVec3> positions;
    //Copy the vertices to the mesh
    for(Q2::Point3f& p: vertices) {
        kmVec3 point;
        kmVec3Fill(&point, p.x, p.y, p.z);
        kmVec3Transform(&point, &point, &rotation);

        positions.push_back(point);
    }

    std::map<kglt::TextureID, uint32_t> mesh_for_texture;

    uint16_t texinfo_idx = 0;
    std::map<uint16_t, MaterialID> tex_info_to_material;
    std::map<std::string, kglt::TextureID> tex_lookup;
    std::map<std::string, std::pair<uint32_t, uint32_t> > texture_dimensions;

    /**
     *  Load the textures and generate materials
     */

    MeshID mid = scene->stage()->new_mesh();
    auto mesh = scene->stage()->mesh(mid);

    std::map<MaterialID, SubMeshIndex> material_to_submesh;

    //We need to hold references here until the materials are attached to the mesh
    std::vector<MaterialPtr> mat_ref_count_holder_;
    std::vector<TexturePtr> tex_ref_count_holder_;

    for(Q2::TextureInfo& tex: textures) {
        auto mat = scene->material(scene->clone_default_material());
        mat_ref_count_holder_.push_back(mat.__object); //Prevent GC until we are done

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

        ProtectedPtr<Texture> texture;
        kglt::TextureID tid;
        if(tex_lookup.find(tex.texture_name) != tex_lookup.end()) {
            tid = tex_lookup[tex.texture_name];
        } else {
            std::string texture_filename = "textures/" + std::string(tex.texture_name) + ".tga";

            try {
                texture = scene->stage()->texture(scene->stage()->new_texture_from_file(texture_filename));
                tex_ref_count_holder_.push_back(texture.__object);
            } catch(IOError& e) {
                //Fallback texture
                L_ERROR("Unable to find texture required by BSP file: " + texture_filename);
                texture = scene->stage()->texture(scene->stage()->new_texture());
                tex_ref_count_holder_.push_back(texture.__object);

                //FIXME: Should be checkerboard, not starfield
                kglt::procedural::texture::starfield(texture.__object, 64, 64);
            }

            tid = texture->id();

            texture->upload(false, true, false, false);

            //We need to store this to divide the texture coordinates later
            texture_dimensions[tex.texture_name].first = texture->width();
            texture_dimensions[tex.texture_name].second = texture->height();

            tex_lookup[tex.texture_name] = tid;
        }

        //Set the texture for unit 0
        mat->technique().pass(0).set_texture_unit(0, tid);
        mat->technique().pass(0).set_blending(BLEND_NONE);

        tex_info_to_material[texinfo_idx] = mat->id();
        texinfo_idx++;

        L_DEBUG(_u("Associated material: {0}").format(mat->id().value()));
        material_to_submesh[mat->id()] = mesh->new_submesh(mat->id(), MESH_ARRANGEMENT_TRIANGLES, true);
    }

    std::cout << "Num textures: " << tex_lookup.size() << std::endl;
    std::cout << "Num submeshes: " << mesh->submesh_ids().size() << std::endl;

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

        //Add a submesh for this face
        SubMesh& sm = mesh->submesh(material_to_submesh[tex_info_to_material[f.texture_info]]);
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
            kmVec3& v1 = positions[tri_idx[0]];
            kmVec3& v2 = positions[tri_idx[1]];
            kmVec3& v3 = positions[tri_idx[2]];

            kmVec3Subtract(&vec1, &v2, &v1);
            kmVec3Subtract(&vec2, &v3, &v1);
            kmVec3Cross(&normal, &vec1, &vec2);
            kmVec3Normalize(&normal, &normal);

            for(uint8_t j = 0; j < 3; ++j) {
                if(container::contains(index_lookup, tri_idx[j])) {
                    //We've already processed this vertex
                    sm.index_data().index(index_lookup[tri_idx[j]]);
                    continue;
                }


                kmVec3& pos = positions[tri_idx[j]];

                //We haven't done this before so calculate the vertex
                float u = pos.x * tex.u_axis.x
                        + pos.y * tex.u_axis.y
                        + pos.z * tex.u_axis.z + tex.u_offset;

                float v = pos.x * tex.v_axis.x
                        + pos.y * tex.v_axis.y
                        + pos.z * tex.v_axis.z + tex.v_offset;

                float w = float(texture_dimensions[tex.texture_name].first);
                float h = float(texture_dimensions[tex.texture_name].second);

                mesh->shared_data().position(pos);
                mesh->shared_data().normal(normal);
                mesh->shared_data().diffuse(kglt::Colour::WHITE);
                mesh->shared_data().tex_coord0(u / w, v / h);
                mesh->shared_data().tex_coord1(u / w, v / h);
                mesh->shared_data().move_next();

                sm.index_data().index(mesh->shared_data().count() - 1);

                //Cache this new vertex in the lookup
                index_lookup[tri_idx[j]] = mesh->shared_data().count() - 1;
            }
        }
    }        

    mesh->shared_data().done();
    for(SubMeshIndex i: mesh->submesh_ids()) {
        //Delete empty submeshes
        if(!mesh->submesh(i).index_data().count()) {
            mesh->delete_submesh(i);
            continue;
        }
        mesh->submesh(i).index_data().done();
    }

    L_DEBUG(_u("Created an actor for mesh").format(mid));
    //Finally, create an actor from the world mesh
    scene->stage()->new_actor(mid);
}

}
}
