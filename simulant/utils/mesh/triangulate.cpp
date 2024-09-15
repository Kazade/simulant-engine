#include <map>

#include "../../meshes/mesh.h"
#include "triangulate.h"

namespace smlt {
namespace utils {

std::vector<uint32_t> triangles_to_triangles(SubMesh* submesh) {
    return submesh->index_data->all();
}

std::vector<uint32_t> triangle_strip_to_triangles(SubMesh* submesh) {
    std::vector<uint32_t> ret;

    auto c = submesh->index_data->count();
    if(c < 3) {
        return ret;
    }

    uint32_t tri_index[3];
    tri_index[1] = submesh->index_data->at(1);
    tri_index[2] = submesh->index_data->at(0);

    for(std::size_t i = 2; i < c; ++i) {
        // Shift and reverse
        tri_index[0] = tri_index[2];
        tri_index[1] = tri_index[1];

        // Add new
        tri_index[2] = submesh->index_data->at(i);

        ret.insert(ret.end(), tri_index, tri_index + 3);
    }

    return ret;
}

std::vector<uint32_t> triangle_fan_to_triangles(SubMesh* submeshes) {
    assert(0 && "Not implemented");
    return std::vector<uint32_t>();
}

typedef std::function<std::vector<uint32_t> (SubMesh*)> ProcessorFunc;

const std::map<MeshArrangement, ProcessorFunc> PROCESSORS = {
    {MESH_ARRANGEMENT_TRIANGLES, triangles_to_triangles},
    {MESH_ARRANGEMENT_TRIANGLE_STRIP, triangle_strip_to_triangles},
    {MESH_ARRANGEMENT_TRIANGLE_FAN, triangle_fan_to_triangles}
};

void triangulate(MeshPtr mesh, std::vector<Vec3> &vertices, std::vector<Triangle> &triangles) {
    vertices.clear();
    triangles.clear(); // Make sure we're starting from a clean slate

    // First, we populate any shared vertices in the output. This is so that when we process submeshes
    // if they use shared vertices we can store the triangle indexes as they are without offsetting into new
    // data. If they don't use shared vertices, we need to append the submesh vertices and offset the indexes.
    for(uint32_t i = 0; i < mesh->vertex_data->count(); ++i) {
        vertices.push_back(
            mesh->vertex_data->attr_as<Vec3>(VERTEX_ATTR_NAME_POSITION, i)
                .value_or(Vec3()));
    }

    for(auto submesh: mesh->each_submesh()) {
        uint32_t offset = 0;

        auto indexes = PROCESSORS.at(submesh->arrangement())(submesh.get());

        // Add the offset if necessary
        if(offset) {
            for(auto& idx: indexes) {
                idx += offset;
            }
        }

        // Push back the triangles
        for(uint32_t i = 0; i < indexes.size(); i+=3) {
            Triangle new_tri;
            new_tri.idx[0] = indexes[i];
            new_tri.idx[1] = indexes[i+1];
            new_tri.idx[2] = indexes[i+2];
            new_tri.mat = submesh->material();

            triangles.push_back(new_tri);
        }
    }
}


}
}
