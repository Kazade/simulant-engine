
#include "shadows.h"
#include "nodes/light.h"
#include "meshes/mesh.h"
#include "meshes/adjacency_info.h"

namespace smlt {

MeshSilhouette::MeshSilhouette(MeshPtr mesh, const Mat4& mesh_transformation, const LightPtr light):
    mesh_(mesh),
    light_direction_or_position_(
        (light->type() == LIGHT_TYPE_DIRECTIONAL) ? light->direction() : light->absolute_position()
    ),
    light_type_(light->type()) {

    mesh_transformation.extract_rotation_and_translation(inverse_mesh_rotation_, inverse_mesh_position_);
    inverse_mesh_rotation_.inverse();
    inverse_mesh_position_ = -inverse_mesh_position_;

    // Directional lights are always in range
    auto within_range = true;

    if(light->type() == LIGHT_TYPE_POINT) {
        // If it's a point light, we see if the meshes aabb intersects the
        // radius of the light
        // FIXME: Do we need to rotate the light position based on the mesh rotation?
        //        the AABB won't be the same if the mesh is rotated, so probably the light
        //        position should be rotated into the mesh identity first...
        within_range = mesh->aabb().intersects_sphere(
            light_direction_or_position_ + inverse_mesh_position_,
            light->range() * 2.0 // Range is radius, intersects_sphere takes diameter
        );
    } else if(light->type() == LIGHT_TYPE_SPOT_LIGHT) {
        // FIXME: need to check spotlight cone for AABB intersection
        assert(0 && "Not Implemented");
    }

    if(within_range) {
        recalculate_silhouette();
    }
}

const std::vector<SilhouetteEdge> &MeshSilhouette::edge_list() {
    return edge_list_;
}

void MeshSilhouette::recalculate_silhouette() {
    /*
     * For directional lights we just need to make sure that
     * we rotate the direction by the inverse of the mesh rotation
     * then calculate the silhouette from there
     *
     * For point lights, we can transform the position of the light by the inverse(?)
     * of the mesh transformation.
     */

    edge_list_.clear();

    if(light_type_ == LIGHT_TYPE_DIRECTIONAL) {
        calculate_directional_silhouette();
    } else if(light_type_ == LIGHT_TYPE_POINT) {
        calculate_point_silhouette();
    } else if(light_type_ == LIGHT_TYPE_SPOT_LIGHT) {
        calculate_spot_silhouette();
    }
}

void MeshSilhouette::calculate_directional_silhouette() {
    auto light_direction = light_direction_or_position_;
    light_direction = light_direction.rotated_by(inverse_mesh_rotation_);
    light_direction = -light_direction; // Reverse to be direction to, rather than from

    AdjacencyInfo* adj = mesh_->adjacency_info.get();
    VertexData* vertices = mesh_->vertex_data.get();

    adj->each_edge([&](std::size_t, const EdgeInfo& edge) {
        auto d1 = edge.normals[0].dot(light_direction);

        // If we have only one triangle, the missing triangle is the opposite of the first
        // (e.g. if the only triangle is facing the light, the edge must be a silhouette,
        // likewise if a triangle is facing away from the light, we must assume that the edge
        // is part of the silhouette)
        auto d2 = (edge.triangle_count == 2) ? edge.normals[1].dot(light_direction) : -d1;

        auto v1 = vertices->position_at<smlt::Vec3>(edge.indexes[0]);
        auto v2 = vertices->position_at<smlt::Vec3>(edge.indexes[1]);

        // If one normal is facing the light and one isn't then
        // store the edge as a silhouette
        if(d1 >= 0 && d2 < 0) {
            edge_list_.push_back(SilhouetteEdge(v1, v2));
        } else if(d1 < 0 && d1 >= 0) {
            edge_list_.push_back(SilhouetteEdge(v2, v1));
        }
    });
}

void MeshSilhouette::calculate_point_silhouette() {
    auto light_position = light_direction_or_position_;

    // Move the light into the mesh's local space (to prevent transforming vertex data)
    light_position += inverse_mesh_position_;

    AdjacencyInfo* adj = mesh_->adjacency_info.get();
    VertexData* vertices = mesh_->vertex_data.get();

    const float eps = std::numeric_limits<float>::epsilon();

    adj->each_edge([&](std::size_t, const EdgeInfo& edge) {
        auto v1 = vertices->position_at<smlt::Vec3>(edge.indexes[0]);
        auto v2 = vertices->position_at<smlt::Vec3>(edge.indexes[1]);

        // Just use one of the edge vertices to determine the light direction
        auto light_direction = light_position - v1;

        auto d1 = edge.normals[0].dot(light_direction);

        // If we have only one triangle, the missing triangle is the opposite of the first
        // (e.g. if the only triangle is facing the light, the edge must be a silhouette,
        // likewise if a triangle is facing away from the light, we must assume that the edge
        // is part of the silhouette)
        auto d2 = (edge.triangle_count == 2) ? edge.normals[1].dot(light_direction) : -d1;

        // If one normal is facing the light and one isn't then
        // store the edge as a silhouette
        if(d1 > eps && d2 <= 0) {
            edge_list_.push_back(SilhouetteEdge(v1, v2));
        } else if(d1 <= 0 && d2 > eps) {
            edge_list_.push_back(SilhouetteEdge(v2, v1));
        }
    });
}

void MeshSilhouette::calculate_spot_silhouette() {
    assert(0 && "Not implemented");
}

}

