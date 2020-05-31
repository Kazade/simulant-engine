#include "ms3d_loader.h"
#include "../vertex_data.h"
#include "../meshes/mesh.h"

namespace smlt {
namespace loaders {

#pragma pack(push, 1)
struct MS3DHeader {
    char id[10];
    int32_t version;
};

struct MS3DVertex {
    uint8_t flags;
    smlt::Vec3 xyz;
    uint8_t bone;
    uint8_t ref_count;
};

struct MS3DTriangle {
    uint16_t flags;
    uint16_t vertexIndices[3];
    smlt::Vec3 vertexNormals[3];
    smlt::Vec3 s;
    smlt::Vec3 t;
    uint8_t smoothingGroup;
    uint8_t groupIndex;
};

struct MS3DGroup {
    uint8_t flags;                              // SELECTED | HIDDEN
    char name[32];                           //
    uint16_t num_triangles;                       //
    std::vector<uint16_t> triangle_indices;      // the groups group the triangles
    char material_index;                      // -1 = no material
};

#pragma pack(pop)


MS3DLoader::MS3DLoader(const unicode& filename, std::shared_ptr<std::istream> data):
    Loader(filename, data) {}

void MS3DLoader::into(Loadable& resource, const LoaderOptions& options) {
    Mesh* mesh = loadable_to<Mesh>(resource);
    assert(mesh && "Tried to load an MS3D file into a non-mesh");

    AssetManager* assets = &mesh->asset_manager();

    mesh->reset(VertexSpecification::DEFAULT);

    MS3DHeader header;
    data_->read((char*) &header.id, sizeof(char) * 10);
    data_->read((char*) &header.version, sizeof(int32_t));

    if(strncmp(header.id, "MS3D000000", 10) != 0) {
        throw std::logic_error("Unsupported MS3D file. ID mismatch");
    }

    uint16_t num_vertices;
    uint16_t num_triangles;
    uint16_t num_groups;
    uint16_t num_materials;

    data_->read((char*) &num_vertices, sizeof(uint16_t));
    std::vector<MS3DVertex> vertices(num_vertices);
    data_->read((char*) &vertices[0], sizeof(MS3DVertex) * num_vertices);

    data_->read((char*) &num_triangles, sizeof(uint16_t));
    std::vector<MS3DTriangle> triangles(num_triangles);
    data_->read((char*) &triangles[0], sizeof(MS3DTriangle) * num_triangles);

    data_->read((char*) &num_groups, sizeof(uint16_t));
    std::vector<MS3DGroup> groups(num_groups);
    for(uint16_t i = 0; i < num_groups; ++i) {
        data_->read((char*) &groups[i].flags, sizeof(uint8_t));
        data_->read((char*) &groups[i].name, sizeof(char) * 32);
        data_->read((char*) &groups[i].num_triangles, sizeof(uint16_t));

        groups[i].triangle_indices.resize(groups[i].num_triangles);

        data_->read((char*) &groups[i].triangle_indices[0], sizeof(uint16_t) * groups[i].num_triangles);
        data_->read((char*) &groups[i].material_index, sizeof(char));
    }

    data_->read((char*) &num_materials, sizeof(uint16_t));
}

}
}
