#pragma once

namespace smlt {


enum ShadowMethod {
    SHADOW_METHOD_STENCIL_DEPTH_FAIL, // Standard
    SHADOW_METHOD_STENCIL_EXCLUSIVE_OR // Really for the Dreamcast modifier volume stuff
};


class MeshAdjacencyInfo {
    /*
     * Stores the adjacency info for a mesh
     *
     * INVALIDATED: When the mesh index or vertex data changes
    */
public:
    MeshAdjacencyInfo(MeshPtr mesh);
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

    MeshSilhouette(MeshPtr mesh, const LightPtr light, const MeshAdjacencyInfo& adjacency);

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
