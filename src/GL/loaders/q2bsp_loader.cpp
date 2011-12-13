#include <cstdint>
#include <fstream>

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

void Q2BSPLoader::load_into(Resource& resource, const std::string& filename) {
    Resource* res_ptr = &resource;
    Scene* scene = dynamic_cast<Scene*>(res_ptr);
    assert(scene && "You passed a Resource that is not a scene to the Scene loader");

    std::ifstream file(filename.c_str(), std::ios::binary);
    if(!file.good()) {
        throw std::runtime_error("Couldn't load the BSP file");
    }

    Q2::Header header;
    file.read((char*)&header, sizeof(Q2::Header));

    if(std::string(header.magic, header.magic + 4) != "IBSP") {
        throw std::runtime_error("Not a valid Q2 map");
    }

    MeshID mid = scene->new_mesh();
    Mesh& mesh = scene->mesh(mid);
    mesh.set_arrangement(MeshArrangement::POINTS);

    int32_t num_vertices = header.lumps[Q2::LumpType::VERTICES].length / sizeof(Q2::Point3f);
    std::vector<Q2::Point3f> vertices(num_vertices);
    file.seekg(header.lumps[Q2::LumpType::VERTICES].offset);
    file.read((char*)&vertices[0], sizeof(Q2::Point3f) * num_vertices);

    for(Q2::Point3f& p: vertices) {
        mesh.add_vertex(p.x * 0.1, p.y * 0.1, p.z * 0.1);
    }

}

}
}
