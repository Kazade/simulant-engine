#pragma once

#include <vector>
#include <functional>
#include "../math/vec3.h"

namespace smlt {

class Mesh;


/*
 * Describes an edge between two vertices that may be shared with up to 2 triangles.
 *
 * If an edge is shared by more than two triangles in a mesh then this struct will only
 * store up to two, ignoring any others. The mesh would be weird if this was the case.
 */
struct EdgeInfo {
    uint32_t indexes[2]; // The indexes that make this edge
    uint32_t triangle_indexes[2]; // The indexes to additional vertices that make triangles
    uint8_t triangle_count; // Either 0, 1, 2
    smlt::Vec3 normals[2]; // Triangle normals
};

class AdjacencyInfo {
public:
    AdjacencyInfo(Mesh* mesh);
    void rebuild();

    uint32_t edge_count() const { return edges_.size(); }
    void each_edge(const std::function<void (std::size_t, const EdgeInfo &)> &cb);
private:
    Mesh* mesh_ = nullptr;
    std::vector<EdgeInfo> edges_;

};

}
