#pragma once

#include "renderers/batching/renderable.h"
#include "meshes/adjacency_info.h"

namespace smlt {

class VertexData;
class IndexData;


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
    SilhouetteEdge(const smlt::Vec3& v1, const smlt::Vec3& v2,
                   uint32_t i1 = 0, uint32_t i2 = 0):
        first(v1), second(v2), first_index(i1), second_index(i2) {

    }

    smlt::Vec3 first;
    smlt::Vec3 second;
    /* Canonical (welded) vertex indices in the source VertexData.
     * Used by MeshSilhouette to walk silhouette edges into ordered closed
     * loops; consumers like ShadowCaster fan-triangulate the cap from those
     * loops rather than iterating every lit triangle. */
    uint32_t first_index = 0;
    uint32_t second_index = 0;

    const smlt::Vec3& operator[](std::size_t i) const {
        return (i == 0) ? first : second;
    }
};

/* A closed silhouette loop — ordered vertices in mesh-local space, where the
 * last vertex implicitly connects back to the first. Each loop is a single
 * "boundary" of the lit region as seen from the light. For most casters there
 * is one loop; meshes with topological holes (e.g. a torus) can produce more
 * than one. */
typedef std::vector<smlt::Vec3> SilhouetteLoop;

/*
 * Builds the edge adjacency for a piece of triangle geometry (vertex + index
 * data) in its local space. The result depends only on the geometry, not on
 * any light or transform, so it can be cached and reused across frames for
 * persistent geometry. Non-triangle arrangements produce an empty list.
 *
 * vertices    - the vertex data containing the triangle positions
 * indices     - the index data (may be null for sequential geometry)
 * index_count - number of indices to consider (0 = use all)
 * arrangement - how the indices/vertices form triangles
 */
void build_silhouette_adjacency(const VertexData* vertices,
                                const IndexData* indices,
                                std::size_t index_count,
                                MeshArrangement arrangement,
                                std::vector<EdgeInfo>& out_edges);

/*
 * Recomputes only the face normals of a previously-built adjacency from the
 * current vertex positions, leaving the (deformation-invariant) topology
 * intact. Use this for animated geometry to avoid rebuilding the full
 * adjacency every frame.
 */
void recompute_silhouette_normals(const VertexData* vertices,
                                  std::vector<EdgeInfo>& edges);

class MeshSilhouette {
    /*
     * Calculates the silhouette edges of a piece of triangle geometry as seen
     * from a particular light, given pre-built edge adjacency (see
     * build_silhouette_adjacency). Edge positions are returned in the geometry's
     * local space; the caller transforms them by `transform` when extruding.
    */
public:

    /*
     * vertices  - the vertex data containing the triangle positions
     * edges     - adjacency built from the same geometry (see
     *             build_silhouette_adjacency); must outlive this object
     * transform - the geometry's world-space transform (used to bring the light
     *             into local space)
     * light     - the light to calculate the silhouette from
    */
    MeshSilhouette(const VertexData* vertices,
                   const std::vector<EdgeInfo>& edges,
                   const Mat4& transform,
                   const LightPtr light);

    /*
     * Returns the list of vertex pairs which make up the calculated silhouette.
     * Returns an empty list if the geometry isn't influenced by the light.
     */
    const std::vector<SilhouetteEdge>& edge_list() const { return edge_list_; }

    /*
     * Returns the silhouette edges stitched into ordered closed loops.
     *
     * For a closed (manifold) caster mesh the lit/unlit boundary is a closed
     * curve, so the edge list always forms one or more closed loops. ShadowCaster
     * uses these to fan-triangulate the front / back caps in O(N_silhouette)
     * instead of iterating every mesh triangle.
     *
     * Edges that don't stitch into a closed loop (e.g. degenerate open meshes)
     * are dropped from this list but remain in edge_list().
     */
    const std::vector<SilhouetteLoop>& loops() const { return loops_; }

    /*
     * True if at least one real (non-phantom) triangle that contributed to the
     * silhouette was lit. Open meshes hit by light from the back produce
     * boundary silhouette edges via the missing-neighbour rule but no actual
     * lit triangles — in that case the caller should emit side quads only,
     * not caps, to match the open-mesh "no shadow" convention.
     */
    bool has_lit_face() const { return has_lit_face_; }

private:
    void calculate_directional_silhouette();
    void calculate_point_silhouette();
    void compute_loops();

    const VertexData* vertices_ = nullptr;
    const std::vector<EdgeInfo>* edges_ = nullptr;
    std::vector<SilhouetteEdge> edge_list_;
    std::vector<SilhouetteLoop> loops_;
    bool has_lit_face_ = false;

    smlt::Vec3 inverse_mesh_position_;
    smlt::Quaternion inverse_mesh_rotation_;
    smlt::Vec3 light_direction_or_position_;
    LightType light_type_;
    float light_range_ = 0.0f; // point light radius (for range culling)
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
