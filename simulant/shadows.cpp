
#include <limits>
#include <tuple>
#include <unordered_map>

#include "shadows.h"
#include "math/aabb.h"
#include "nodes/light.h"
#include "vertex_data.h"

namespace smlt {

static Vec3 compute_face_normal(const VertexData* vertices,
                                uint32_t a, uint32_t b, uint32_t c) {
    auto va = vertices->position_at<smlt::Vec3>(a);
    auto vb = vertices->position_at<smlt::Vec3>(b);
    auto vc = vertices->position_at<smlt::Vec3>(c);
    auto v1 = *vb - *va;
    auto v2 = *vc - *va;
    return v1.cross(v2).normalized();
}

void build_silhouette_adjacency(const VertexData* vertices,
                                const IndexData* indices,
                                std::size_t index_count,
                                MeshArrangement arrangement,
                                std::vector<EdgeInfo>& out_edges) {
    out_edges.clear();

    if(!vertices) {
        return;
    }

    /* Only 3D positions are supported for silhouette generation */
    if(vertices->vertex_specification().position_attribute != VERTEX_ATTRIBUTE_3F) {
        return;
    }

    /* Only triangle arrangements contribute to a silhouette */
    if(arrangement != MESH_ARRANGEMENT_TRIANGLES &&
       arrangement != MESH_ARRANGEMENT_TRIANGLE_STRIP &&
       arrangement != MESH_ARRANGEMENT_TRIANGLE_FAN) {
        return;
    }

    const uint32_t vertex_count = vertices->count();

    auto calculate_normal = [&vertices](uint32_t a, uint32_t b, uint32_t c) -> smlt::Vec3 {
        return compute_face_normal(vertices, a, b, c);
    };

    /* Resolve the index count to iterate over */
    std::size_t count;
    if(indices) {
        count = index_count ? index_count : indices->count();
    } else {
        count = index_count ? index_count : vertex_count;
    }

    /* Fetch the i'th index, whether the geometry is indexed or sequential */
    auto fetch = [&](std::size_t i) -> uint32_t {
        return indices ? indices->at((uint32_t)i) : (uint32_t)i;
    };

    typedef std::tuple<uint32_t, uint32_t> edge_pair;
    typedef std::tuple<float, float, float> vec_tuple;
    std::unordered_map<vec_tuple, uint32_t> position_map;
    std::unordered_map<edge_pair, uint32_t> edge_triangles;

    auto to_tuple = [](const Vec3& v) {
        return std::make_tuple(v.x, v.y, v.z);
    };

    /* Weld vertices at the same position to a canonical index, then record the
     * third vertex for each directed edge. Mirrors AdjacencyInfo::rebuild. */
    auto process_triangle = [&](uint32_t a, uint32_t b, uint32_t c) {
        if(a >= vertex_count || b >= vertex_count || c >= vertex_count) {
            return;
        }

        auto v1 = to_tuple(*vertices->position_at<smlt::Vec3>(a));
        auto v2 = to_tuple(*vertices->position_at<smlt::Vec3>(b));
        auto v3 = to_tuple(*vertices->position_at<smlt::Vec3>(c));

        a = (position_map.count(v1)) ? position_map[v1] : position_map.insert(std::make_pair(v1, a)).first->second;
        b = (position_map.count(v2)) ? position_map[v2] : position_map.insert(std::make_pair(v2, b)).first->second;
        c = (position_map.count(v3)) ? position_map[v3] : position_map.insert(std::make_pair(v3, c)).first->second;

        edge_triangles.insert(std::make_pair(std::make_pair(a, b), c));
        edge_triangles.insert(std::make_pair(std::make_pair(b, c), a));
        edge_triangles.insert(std::make_pair(std::make_pair(c, a), b));
    };

    if(arrangement == MESH_ARRANGEMENT_TRIANGLES) {
        for(std::size_t i = 0; i + 3 <= count; i += 3) {
            process_triangle(fetch(i), fetch(i + 1), fetch(i + 2));
        }
    } else if(arrangement == MESH_ARRANGEMENT_TRIANGLE_STRIP) {
        for(std::size_t i = 2; i < count; ++i) {
            if(i % 2 == 0) {
                process_triangle(fetch(i - 2), fetch(i - 1), fetch(i));
            } else {
                process_triangle(fetch(i), fetch(i - 1), fetch(i - 2));
            }
        }
    } else { // MESH_ARRANGEMENT_TRIANGLE_FAN
        if(count >= 3) {
            auto hub = fetch(0);
            for(std::size_t i = 2; i < count; ++i) {
                process_triangle(hub, fetch(i - 1), fetch(i));
            }
        }
    }

    out_edges.reserve(edge_triangles.size());

    std::unordered_map<edge_pair, std::size_t> edge_lookup;

    for(auto& p: edge_triangles) {
        /* Checking only for reversed is intentional because of polygon winding;
         * an edge can only be shared if it's in the opposite direction. */
        auto i0 = std::get<0>(p.first);
        auto i1 = std::get<1>(p.first);

        auto reversed = std::make_tuple(i1, i0);
        auto existing = edge_lookup.find(reversed);

        if(existing != edge_lookup.end()) {
            auto& existing_edge = out_edges[existing->second];
            existing_edge.triangle_indexes[1] = p.second;
            existing_edge.triangle_count = 2;
            existing_edge.normals[1] = calculate_normal(
                existing_edge.indexes[1],
                existing_edge.indexes[0],
                existing_edge.triangle_indexes[1]
            );
        } else {
            EdgeInfo new_info;
            new_info.indexes[0] = i0;
            new_info.indexes[1] = i1;
            new_info.triangle_indexes[0] = p.second;
            new_info.triangle_count = 1;
            new_info.normals[0] = calculate_normal(new_info.indexes[0], new_info.indexes[1], new_info.triangle_indexes[0]);

            out_edges.push_back(new_info);
            edge_lookup.insert(std::make_pair(std::make_tuple(i0, i1), out_edges.size() - 1));
        }
    }
}

void recompute_silhouette_normals(const VertexData* vertices,
                                  std::vector<EdgeInfo>& edges) {
    if(!vertices ||
       vertices->vertex_specification().position_attribute != VERTEX_ATTRIBUTE_3F) {
        return;
    }

    /* Topology (indexes/triangle_indexes/triangle_count) is invariant under
     * deformation, but the face normals are not — recompute them from the
     * current vertex positions. Must mirror the normal winding used in
     * build_silhouette_adjacency. */
    for(auto& edge: edges) {
        edge.normals[0] = compute_face_normal(
            vertices, edge.indexes[0], edge.indexes[1], edge.triangle_indexes[0]);
        if(edge.triangle_count == 2) {
            edge.normals[1] = compute_face_normal(
                vertices, edge.indexes[1], edge.indexes[0], edge.triangle_indexes[1]);
        }
    }
}

MeshSilhouette::MeshSilhouette(const VertexData* vertices,
                               const std::vector<EdgeInfo>& edges,
                               const Mat4& transform,
                               const LightPtr light):
    vertices_(vertices),
    edges_(&edges),
    light_direction_or_position_(
          (light->light_type() == LIGHT_TYPE_DIRECTIONAL) ? light->direction() : light->transform->position()
    ),
    light_type_(light->light_type()),
    light_range_(light->range()) {

    transform.extract_rotation_and_translation(inverse_mesh_rotation_, inverse_mesh_position_);
    inverse_mesh_rotation_.inverse();
    inverse_mesh_position_ = -inverse_mesh_position_;

    if(!vertices_ || edges_->empty()) {
        return;
    }

    if(light_type_ == LIGHT_TYPE_DIRECTIONAL) {
        calculate_directional_silhouette();
    } else if(light_type_ == LIGHT_TYPE_POINT) {
        calculate_point_silhouette();
    }
    /* Spotlights are not yet supported */
}

void MeshSilhouette::calculate_directional_silhouette() {
    auto light_direction = light_direction_or_position_;
    light_direction = light_direction.rotated_by(inverse_mesh_rotation_);
    light_direction = -light_direction; // Reverse to be direction to, rather than from

    for(const auto& edge: *edges_) {
        auto d1 = edge.normals[0].dot(light_direction);

        // If we have only one triangle, the missing triangle is the opposite of the first
        // (e.g. if the only triangle is facing the light, the edge must be a silhouette,
        // likewise if a triangle is facing away from the light, we must assume that the edge
        // is part of the silhouette)
        auto d2 = (edge.triangle_count == 2) ? edge.normals[1].dot(light_direction) : -d1;

        auto v1 = vertices_->position_at<smlt::Vec3>(edge.indexes[0]);
        auto v2 = vertices_->position_at<smlt::Vec3>(edge.indexes[1]);

        // If one normal is facing the light and one isn't then
        // store the edge as a silhouette
        if(d1 >= 0 && d2 < 0) {
            edge_list_.push_back(SilhouetteEdge(*v1, *v2));
        } else if(d1 < 0 && d2 >= 0) {
            edge_list_.push_back(SilhouetteEdge(*v2, *v1));
        }
    }
}

void MeshSilhouette::calculate_point_silhouette() {
    auto light_position = light_direction_or_position_;

    // Move the light into the geometry's local space (to avoid transforming the
    // vertex data). For a model matrix T*R, world->local is R⁻¹·(world − T), so
    // we need both the inverse translation and the inverse rotation. Translation
    // alone was wrong for rotated casters.
    light_position += inverse_mesh_position_;
    light_position = light_position.rotated_by(inverse_mesh_rotation_);

    // Cull the whole piece of geometry if it's outside the light's range.
    // intersects_sphere takes a diameter, range is a radius.
    AABB bounds(*vertices_);
    if(!bounds.intersects_sphere(light_position, light_range_ * 2.0f)) {
        return;
    }

    const float eps = std::numeric_limits<float>::epsilon();

    for(const auto& edge: *edges_) {
        auto v1 = vertices_->position_at<smlt::Vec3>(edge.indexes[0]);
        auto v2 = vertices_->position_at<smlt::Vec3>(edge.indexes[1]);

        // Just use one of the edge vertices to determine the light direction
        auto light_direction = light_position - *v1;

        auto d1 = edge.normals[0].dot(light_direction);
        auto d2 = (edge.triangle_count == 2) ? edge.normals[1].dot(light_direction) : -d1;

        if(d1 > eps && d2 <= 0) {
            edge_list_.push_back(SilhouetteEdge(*v1, *v2));
        } else if(d1 <= 0 && d2 > eps) {
            edge_list_.push_back(SilhouetteEdge(*v2, *v1));
        }
    }
}

}
