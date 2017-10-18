#include "adjacency_info.h"

#include "mesh.h"
#include "submesh.h"

namespace smlt {

AdjacencyInfo::AdjacencyInfo(Mesh* mesh):
    mesh_(mesh) {

}

void AdjacencyInfo::rebuild() {
    auto to_tuple = [](const Vec3& v) {
        return std::make_tuple(v.x, v.y, v.z);
    };

    if(!mesh_) {
        return;
    }

    edges_.clear();

    typedef std::tuple<uint32_t, uint32_t> edge_pair;
    typedef std::tuple<float, float, float> vec_tuple;
    std::unordered_map<vec_tuple, uint32_t> position_map;
    std::unordered_map<edge_pair, uint32_t> edge_triangles;

    // FIXME: handle other types
    assert(mesh_->shared_data->specification().position_attribute == VERTEX_ATTRIBUTE_3F);

    mesh_->each([&](const std::string&, SubMeshPtr submesh) {
        if(!submesh->contributes_to_edge_list()) {
            // Ignore submeshes which don't contribute to the edge list
            return;
        }

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            // FIXME: What if it's a vec2?
            auto v1 = to_tuple(mesh_->shared_data->position_at<smlt::Vec3>(a));
            auto v2 = to_tuple(mesh_->shared_data->position_at<smlt::Vec3>(b));
            auto v3 = to_tuple(mesh_->shared_data->position_at<smlt::Vec3>(c));

            // If this vertex already exist, use the first known index (or insert this as the first known index)
            a = (position_map.count(v1)) ? position_map[v1] : position_map.insert(std::make_pair(v1, a)).second;
            b = (position_map.count(v2)) ? position_map[v2] : position_map.insert(std::make_pair(v2, b)).second;
            c = (position_map.count(v3)) ? position_map[v3] : position_map.insert(std::make_pair(v3, c)).second;

            edge_triangles.insert(std::make_pair(std::make_pair(a, b), c));
            edge_triangles.insert(std::make_pair(std::make_pair(b, c), a));
            edge_triangles.insert(std::make_pair(std::make_pair(c, a), b));
        });
    });

    /*
       (0, 1) -> 2
       (1, 2) -> 3,
       (1, 0) -> 4
    */

    for(auto& p: edge_triangles) {
        auto reversed = std::make_pair(
            std::get<1>(p.first),
            std::get<0>(p.first)
        );

        /* If the edge exists in reversed order, then update that */
        auto existing = std::find_if(
            edges_.begin(), edges_.end(),
            [&reversed](const EdgeInfo& e) -> bool {
                return e.indexes[0] == reversed.first &&
                       e.indexes[1] == reversed.second;
            }
        );

        if(existing != edges_.end()) {
            existing->triangle_indexes[1] = p.second;
            existing->triangle_count = 2;
        } else {
            // Add the new edge
            EdgeInfo new_info;
            new_info.indexes[0] = std::get<0>(p.first);
            new_info.indexes[1] = std::get<1>(p.first);
            new_info.triangle_indexes[0] = p.second;
            new_info.triangle_count = 1;

            edges_.push_back(new_info);
        }
    }
}

void AdjacencyInfo::each_edge(std::function<void (std::size_t, const EdgeInfo& edge)>& cb) {
    std::size_t i = 0;
    for(auto& edge: edges_) {
        cb(i++, edge);
    }
}

/*
 * Given an edge, this returns the two normals of the triangles which share it.
 * If a triangle doesn't exist then a zero-vector will be returned
 */
std::pair<smlt::Vec3, smlt::Vec3> AdjacencyInfo::edge_normals(const EdgeInfo& edge) const {
    assert(0 && "Not Implemented");
}

}
