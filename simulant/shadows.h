#pragma once

#include "renderers/batching/renderable.h"

namespace smlt {


enum ShadowMethod {
    SHADOW_METHOD_STENCIL_DEPTH_FAIL, // Standard
    SHADOW_METHOD_STENCIL_EXCLUSIVE_OR // Really for the Dreamcast modifier volume stuff
};

enum ShadowCast {
    SHADOW_CAST_ALWAYS,
    SHADOW_CAST_NEVER
};

enum ShadowReceive {
    SHADOW_RECEIVE_ALWAYS,
    SHADOW_RECEIVE_NEVER
};

struct SilhouetteEdge {
    SilhouetteEdge(const smlt::Vec3& v1, const smlt::Vec3& v2):
        first(v1), second(v2) {

    }

    smlt::Vec3 first;
    smlt::Vec3 second;

    const smlt::Vec3& operator[](std::size_t i) const {
        if(i == 0) {
            return first;
        } else if(i == 1) {
            return second;
        } else {
            throw std::out_of_range("Invalid edge index");
        }
    }
};

class MeshSilhouette {
    /*
     * Stores the chain of edges that form a silohette from a particular light
     *
     * INVALIDATED:
     *  - When the mesh adjacency is invalided
     *  - When the light moves
    */
public:

    /*
     * mesh - The mesh that this silhouette is for
     * mesh_rotation - The rotation of the mesh to calculate (normally from the Actor)
     * light - The light to calculate the silhoutte from
    */
    MeshSilhouette(MeshPtr mesh, const Mat4& mesh_transformation, const LightPtr light);

    /*
     * Returns the list of vertex pairs which make up the calculated silhouette. Returns
     * an empty list if the mesh isn't influenced by the light
     */
    const std::vector<SilhouetteEdge>& edge_list();

private:
    void recalculate_silhouette();
    void calculate_directional_silhouette();
    void calculate_point_silhouette();
    void calculate_spot_silhouette();

    std::vector<SilhouetteEdge> edge_list_;

    smlt::MeshPtr mesh_;
    smlt::Vec3 inverse_mesh_position_;
    smlt::Quaternion inverse_mesh_rotation_;
    smlt::Vec3 light_direction_or_position_;
    LightType light_type_;
};

class ShadowVolumeManager {
    /*
     * Calculates and stores the shadow volumes for a stage. ShadowManager::update should be
     * called with the visible lights and shadow-casting renderables each time a camera view
     * is rendered.
     *
     * Shadow volumes will not be updated in the following situations:
     *
     * 1. The light <> renderable volume has already been calculated this frame
     * 2. The light and renderable haven't moved since a previous frame
     *
     * ShadowVolumes are destroyed when the light or renderable are destroyed.
     */
public:
    void update(uint64_t frame_id, const std::vector<Light>& lights, const std::vector<RenderablePtr>& renderables);

};




}
