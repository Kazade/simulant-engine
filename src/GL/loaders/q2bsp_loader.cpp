#include <cstdint>
#include <fstream>
#include <iostream>

#include "../window.h"
#include "../scene.h"
#include "../mesh.h"
#include "../types.h"

#include "q2bsp_loader.h"

namespace GL  {
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

void Q2BSPLoader::into(Resource& resource) {
    Resource* res_ptr = &resource;
    Scene* scene = dynamic_cast<Scene*>(res_ptr);
    assert(scene && "You passed a Resource that is not a scene to the Scene loader");

    std::ifstream file(filename_.c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the BSP file: " + filename_);
    }

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    MeshID mid = scene->new_mesh();
    Mesh& mesh = scene->mesh(mid);
    //mesh.set_arrangement(MeshArrangement::POINTS);

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
        mesh.add_vertex(p.x * 0.1, p.y * 0.1, p.z * 0.1);
    }

    std::map<std::string, GL::TextureID> tex_lookup;
    for(Q2::TextureInfo& tex: textures) {
        if(tex_lookup.find(tex.texture_name) != tex_lookup.end()) continue;

        //Load texture
        GL::TextureID tid = scene->new_texture();
        Texture& texture = scene->texture(tid);
        scene->window()->loader_for(std::string(tex.texture_name) + ".tga")->into(texture);
        texture.upload();
        tex_lookup[tex.texture_name] = tid;
    }

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

        for(int32_t i = 1; i < indexes.size() - 1; ++i) {
            std::vector<uint32_t> tri_idx;
            tri_idx.push_back(indexes[0]);
            tri_idx.push_back(indexes[i+1]);
            tri_idx.push_back(indexes[i]);

            Triangle& tri = mesh.add_triangle(tri_idx[0], tri_idx[1], tri_idx[2]);
            tri.tex_id = tex_lookup[tex.texture_name];

            for(int32_t j = 0; j < 3; ++j) {
                float u = mesh.vertex(tri_idx[j]).x * tex.u_axis.x
                        + mesh.vertex(tri_idx[j]).y * tex.u_axis.y
                        + mesh.vertex(tri_idx[j]).z * tex.u_axis.z + tex.u_offset;

                float v = mesh.vertex(tri_idx[j]).x * tex.v_axis.x
                        + mesh.vertex(tri_idx[j]).y * tex.v_axis.y
                        + mesh.vertex(tri_idx[j]).z * tex.v_axis.z + tex.v_offset;

                tri.uv[j].x = u;
                tri.uv[j].y = v;
            }
        }
    }
}

}
}
