
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

    compute_loops();
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

        // d < 0 means the face normal opposes the away-from-light direction —
        // i.e. it faces the light. Track whether ANY real triangle is lit, so
        // ShadowCaster can suppress cap emission for entirely-unlit open meshes
        // (whose silhouette comes only from phantom neighbours).
        if(d1 < 0.0f) has_lit_face_ = true;
        if(edge.triangle_count == 2 && d2 < 0.0f) has_lit_face_ = true;

        auto v1 = vertices_->position_at<smlt::Vec3>(edge.indexes[0]);
        auto v2 = vertices_->position_at<smlt::Vec3>(edge.indexes[1]);

        // If one normal is facing the light and one isn't then
        // store the edge as a silhouette
        if(d1 >= 0 && d2 < 0) {
            edge_list_.push_back(SilhouetteEdge(*v1, *v2, edge.indexes[0], edge.indexes[1]));
        } else if(d1 < 0 && d2 >= 0) {
            edge_list_.push_back(SilhouetteEdge(*v2, *v1, edge.indexes[1], edge.indexes[0]));
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

        // For point lights light_direction points TOWARDS the light, so d > 0
        // means the face is lit. Track whether any real triangle is lit so the
        // caller can decide whether to emit caps.
        if(d1 > eps) has_lit_face_ = true;
        if(edge.triangle_count == 2 && d2 > eps) has_lit_face_ = true;

        if(d1 > eps && d2 <= 0) {
            edge_list_.push_back(SilhouetteEdge(*v1, *v2, edge.indexes[0], edge.indexes[1]));
        } else if(d1 <= 0 && d2 > eps) {
            edge_list_.push_back(SilhouetteEdge(*v2, *v1, edge.indexes[1], edge.indexes[0]));
        }
    }
}

void MeshSilhouette::compute_loops() {
    /* Walk the silhouette edges into ordered closed loops. Each canonical
     * vertex in a manifold silhouette appears in exactly two silhouette edges,
     * so the walk is O(N_edges) using a vertex-to-edges index. Edges that
     * don't stitch into a closed cycle are skipped. */
    if(edge_list_.empty()) {
        return;
    }

    std::unordered_map<uint32_t, std::vector<uint32_t>> vtx_to_edges;
    vtx_to_edges.reserve(edge_list_.size() * 2);
    for(uint32_t i = 0; i < (uint32_t)edge_list_.size(); ++i) {
        vtx_to_edges[edge_list_[i].first_index].push_back(i);
        vtx_to_edges[edge_list_[i].second_index].push_back(i);
    }

    std::vector<bool> visited(edge_list_.size(), false);

    for(uint32_t start = 0; start < (uint32_t)edge_list_.size(); ++start) {
        if(visited[start]) continue;

        SilhouetteLoop loop;
        const uint32_t start_vtx = edge_list_[start].first_index;
        uint32_t prev_vtx = start_vtx;
        uint32_t cur_vtx  = edge_list_[start].second_index;
        loop.push_back(edge_list_[start].first);
        loop.push_back(edge_list_[start].second);
        visited[start] = true;

        bool closed = false;
        while(true) {
            if(cur_vtx == start_vtx) {
                // The last pushed vertex was a duplicate of the loop's start;
                // drop it so the loop's last element isn't a repeat.
                loop.pop_back();
                closed = true;
                break;
            }

            auto it = vtx_to_edges.find(cur_vtx);
            if(it == vtx_to_edges.end()) break;

            uint32_t next_edge = UINT32_MAX;
            uint32_t next_vtx  = UINT32_MAX;
            Vec3     next_pos;
            for(uint32_t e: it->second) {
                if(visited[e]) continue;
                const auto& ed = edge_list_[e];
                if(ed.first_index == cur_vtx && ed.second_index != prev_vtx) {
                    next_edge = e;
                    next_vtx  = ed.second_index;
                    next_pos  = ed.second;
                    break;
                }
                if(ed.second_index == cur_vtx && ed.first_index != prev_vtx) {
                    next_edge = e;
                    next_vtx  = ed.first_index;
                    next_pos  = ed.first;
                    break;
                }
            }

            if(next_edge == UINT32_MAX) break; // dead end (open chain)

            visited[next_edge] = true;
            prev_vtx = cur_vtx;
            cur_vtx  = next_vtx;
            loop.push_back(next_pos);
        }

        if(closed && loop.size() >= 3) {
            loops_.push_back(std::move(loop));
        }
    }
}

}
