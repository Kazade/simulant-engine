#include "../logging.h"

#include "adjacency_info.h"

#include "mesh.h"
#include "submesh.h"

namespace smlt {

AdjacencyInfo::AdjacencyInfo(Mesh* mesh):
    mesh_(mesh) {

}

void AdjacencyInfo::rebuild() {
    L_DEBUG("Generating adjacency info");

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
    if(mesh_->vertex_data->vertex_specification().position_attribute != VERTEX_ATTRIBUTE_3F) {
        L_WARN("Adjacency info currently only supported on 3D vertices");
        return;
    }

    auto vertices = mesh_->vertex_data.get();
    auto calculate_normal = [&vertices](uint32_t a, uint32_t b, uint32_t c) -> smlt::Vec3 {
        auto va = vertices->position_at<smlt::Vec3>(a);
        auto vb = vertices->position_at<smlt::Vec3>(b);
        auto vc = vertices->position_at<smlt::Vec3>(c);
        auto v1 = *vb - *va;
        auto v2 = *vc - *va;
        return v1.cross(v2).normalized();
    };

    auto size = mesh_->vertex_data->count();

    for(auto submesh: mesh_->each_submesh()) {
        if(!submesh->contributes_to_edge_list()) {
            // Ignore submeshes which don't contribute to the edge list
            return;
        }

        submesh->each_triangle([&](uint32_t a, uint32_t b, uint32_t c) {
            // FIXME: What if it's a vec2?
            assert(a < size);
            assert(b < size);
            assert(c < size);

            auto v1 = to_tuple(*vertices->position_at<smlt::Vec3>(a));
            auto v2 = to_tuple(*vertices->position_at<smlt::Vec3>(b));
            auto v3 = to_tuple(*vertices->position_at<smlt::Vec3>(c));

            // If this vertex already exist, use the first known index (or insert this as the first known index)
            a = (position_map.count(v1)) ? position_map[v1] : position_map.insert(std::make_pair(v1, a)).first->second;
            b = (position_map.count(v2)) ? position_map[v2] : position_map.insert(std::make_pair(v2, b)).first->second;
            c = (position_map.count(v3)) ? position_map[v3] : position_map.insert(std::make_pair(v3, c)).first->second;

            edge_triangles.insert(std::make_pair(std::make_pair(a, b), c));
            edge_triangles.insert(std::make_pair(std::make_pair(b, c), a));
            edge_triangles.insert(std::make_pair(std::make_pair(c, a), b));
        });
    }

    /*
       (0, 1) -> 2
       (1, 2) -> 3,
       (1, 0) -> 4
    */

    // For performance
    edges_.reserve(edge_triangles.size());

    std::unordered_map<std::tuple<uint32_t, uint32_t>, std::size_t> edge_lookup;

    for(auto& p: edge_triangles) {
        /* Checking only for reversed is intentional because of polygon winding
         * an edge can only be shared if it's in the opposite direction */
        auto i0 = std::get<0>(p.first);
        auto i1 = std::get<1>(p.first);

        auto reversed = std::make_tuple(i1, i0);

        /* If the edge exists in reversed order, then update that */
        auto existing = edge_lookup.find(reversed);

        if(existing != edge_lookup.end()) {
            auto& existing_edge = edges_[existing->second];
            existing_edge.triangle_indexes[1] = p.second;
            existing_edge.triangle_count = 2;
            existing_edge.normals[1] = calculate_normal(
                existing_edge.indexes[1],
                existing_edge.indexes[0],
                existing_edge.triangle_indexes[1]
            );
        } else {
            // Add the new edge
            EdgeInfo new_info;
            new_info.indexes[0] = i0;
            new_info.indexes[1] = i1;
            new_info.triangle_indexes[0] = p.second;
            new_info.triangle_count = 1;
            new_info.normals[0] = calculate_normal(new_info.indexes[0], new_info.indexes[1], new_info.triangle_indexes[0]);

            edges_.push_back(new_info);

            auto t = std::make_tuple(new_info.indexes[0], new_info.indexes[1]);
            edge_lookup.insert(std::make_pair(t, edges_.size() - 1));
        }
    }
}

void AdjacencyInfo::each_edge(const std::function<void (std::size_t, const EdgeInfo& edge)>& cb) {
    std::size_t i = 0;
    for(auto& edge: edges_) {
        cb(i++, edge);
    }
}

}
