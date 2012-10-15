#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <boost/algorithm/string/trim.hpp>

#include "../window.h"
#include "../scene.h"
#include "../mesh.h"
#include "../types.h"

#include "kazbase/logging/logging.h"
#include "kazbase/string.h"
#include "kazmath/vec3.h"
#include "kazmath/mat4.h"

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

typedef std::map<std::string, std::string> EntityProperties;

void parse_entities(const std::string& entity_string, std::vector<EntityProperties>& entities) {
    bool inside_entity = false;
    EntityProperties current;
    std::string key, value;
    bool inside_key = false, inside_value = false, key_done_for_this_line = false;
    for(char c: entity_string) {
        if(c == '{') {
            assert(!inside_entity);
            inside_entity = true;
            current.clear();
        }
        else if(c == '}') {
            assert(inside_entity);
            inside_entity = false;
            entities.push_back(current);
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

kmVec3 find_player_spawn_point(std::vector<EntityProperties>& entities) {
    for(EntityProperties p: entities) {
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

void add_lights_to_scene(Scene& scene, const std::vector<EntityProperties>& entities) {
    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    for(EntityProperties props: entities) {
        if(props["classname"] == "light") {
            kmVec3 pos;
            std::istringstream origin(props["origin"]);
            origin >> pos.x >> pos.y >> pos.z;

            kglt::Light& new_light = kglt::return_new_light(scene);

            kmVec3Transform(&pos, &pos, &rotation);
            new_light.move_to(pos.x, pos.y, pos.z);

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
                new_light.set_diffuse(diffuse);
            }


            std::cout << "Creating light at: " << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
            new_light.set_attenuation_from_range(range);
        }
    }
}

void Q2BSPLoader::into(Loadable& resource) {
    Loadable* res_ptr = &resource;
    Scene* scene = dynamic_cast<Scene*>(res_ptr);
    assert(scene && "You passed a Resource that is not a scene to the Scene loader");

    std::ifstream file(filename_.c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the BSP file: " + filename_);
    }

    //Needed because the Quake 2 coord system is weird
    kmMat4 rotation;
    kmMat4RotationX(&rotation, kmDegreesToRadians(-90.0f));

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    MeshID mid = scene->new_mesh();
    Mesh& mesh = scene->mesh(mid);
    //mesh.set_arrangement(MeshArrangement::POINTS);

    std::vector<char> entity_buffer(header.lumps[Q2::LumpType::ENTITIES].length);
    file.seekg(header.lumps[Q2::LumpType::ENTITIES].offset);
    file.read(&entity_buffer[0], sizeof(char) * header.lumps[Q2::LumpType::ENTITIES].length);
    std::string entity_string(entity_buffer.begin(), entity_buffer.end());


    std::vector<EntityProperties> entities;
    parse_entities(entity_string, entities);
    scene->camera().position() = find_player_spawn_point(entities);
    kmVec3Transform(&scene->camera().position(), &scene->camera().position(), &rotation);

    add_lights_to_scene(*scene, entities);

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


    //Copy the vertices to the mesh
    for(Q2::Point3f& p: vertices) {
        kmVec3 point;
        kmVec3Fill(&point, p.x, p.y, p.z);
        kmVec3Transform(&point, &point, &rotation);
        mesh.add_vertex(point.x, point.y, point.z);
    }

    std::map<std::string, kglt::TextureID> tex_lookup;
    std::map<kglt::TextureID, uint32_t> mesh_for_texture;
    std::map<std::string, std::pair<uint32_t, uint32_t> > texture_dimensions;

    for(Q2::TextureInfo& tex: textures) {
        MaterialID material_id = scene->new_material(scene->default_material()); //Duplicate the default material
        Material& mat = scene->material(material_id);

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

        if(tex_lookup.find(tex.texture_name) != tex_lookup.end()) continue;

        //Load texture
        kglt::TextureID tid = scene->new_texture();
        Texture& texture = scene->texture(tid);

        //HACK!
        scene->window().loader_for("textures/" + std::string(tex.texture_name) + ".tga")->into(texture);

        //We need to store this to divide the texture coordinates later
        texture_dimensions[tex.texture_name].first = texture.width();
        texture_dimensions[tex.texture_name].second = texture.height();

        texture.upload();
        tex_lookup[tex.texture_name] = tid;

        //Set the texture for unit 0
        mat.technique().pass(0).set_texture_unit(0, tid);
        mat.technique().pass(0).set_blending(BLEND_NONE);

        //Create a submesh for each texture, set it to use the parent mesh's verticces
        mesh_for_texture[tid] = mesh.add_submesh(true);
        Mesh& new_submesh = mesh.submesh(mesh_for_texture[tid]);
        new_submesh.apply_material(material_id);
    }

    std::cout << "Num textures: " << tex_lookup.size() << std::endl;
    std::cout << "Num submeshes: " << mesh.submeshes().size() << std::endl;

    for(Q2::Face& f: faces) {
        std::vector<uint32_t> indexes;
        for(uint32_t i = f.first_edge; i < f.first_edge + f.num_edges; ++i) {
            int32_t edge_idx = face_edges[i];
            if(edge_idx > 0) {
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.a);
                indexes.push_back(e.b);
            } else {
                edge_idx = -edge_idx;
                Q2::Edge& e = edges[edge_idx];
                indexes.push_back(e.b);
                indexes.push_back(e.a);
            }
        }

        Q2::TextureInfo& tex = textures[f.texture_info];
        //std::cout << tex.texture_name << std::endl;
        Mesh& texture_mesh = mesh.submesh(mesh_for_texture[tex_lookup[tex.texture_name]]);
        for(int32_t i = 1; i < (int32_t) indexes.size() - 1; ++i) {
            uint32_t tri_idx[] = {
                indexes[0],
                indexes[i+1],
                indexes[i]
            };

            Triangle& tri = texture_mesh.add_triangle(tri_idx[0], tri_idx[1], tri_idx[2]);

            Vec3 normal;
            Vec3 vec1, vec2;
            Vec3& v1 = mesh.vertex(tri.index(0));
            Vec3& v2 = mesh.vertex(tri.index(1));
            Vec3& v3 = mesh.vertex(tri.index(2));

            kmVec3Subtract(&vec1, &v2, &v1);
            kmVec3Subtract(&vec2, &v3, &v1);
            kmVec3Cross(&normal, &vec1, &vec2);
            kmVec3Normalize(&normal, &normal);

            tri.set_surface_normal(normal.x, normal.y, normal.z);

            for(int32_t j = 0; j < 3; ++j) {
                float u = mesh.vertex(tri_idx[j]).x * tex.u_axis.x
                        + mesh.vertex(tri_idx[j]).y * tex.u_axis.y
                        + mesh.vertex(tri_idx[j]).z * tex.u_axis.z + tex.u_offset;

                float v = mesh.vertex(tri_idx[j]).x * tex.v_axis.x
                        + mesh.vertex(tri_idx[j]).y * tex.v_axis.y
                        + mesh.vertex(tri_idx[j]).z * tex.v_axis.z + tex.v_offset;

                float w = float(texture_dimensions[tex.texture_name].first);
                float h = float(texture_dimensions[tex.texture_name].second);

                tri.set_uv(j, u / w, v / h);
            }
        }
    }

    L_DEBUG("Compiling meshes");
    for(Mesh::ptr m: mesh.submeshes()) {
        m->done();
    }
}

}
}
