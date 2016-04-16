#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include <kazbase/signals.h>

#include "../partitioner.h"
#include "../interfaces.h"
#include "../mesh.h"
#include "octree.h"

namespace kglt {

class Renderable;

typedef std::shared_ptr<Renderable> RenderablePtr;


class Polygon:
    public BoundableEntity {

};

class StaticChunk:
    public Renderable {
public:
    StaticChunk(Stage* stage):
        mesh_(MeshID(), stage) {}

    typedef std::shared_ptr<StaticChunk> ptr;

    const AABB aabb() const { return mesh_.aabb(); }
    const AABB transformed_aabb() const { return mesh_.aabb(); }
    const VertexData& vertex_data() const;
    const IndexData& index_data() const;
    const MeshArrangement arrangement() const;

    void _update_vertex_array_object();
    void _bind_vertex_array_object();

    //FIXME: Damn... we need to have static chunks organized by render priority
    RenderPriority render_priority() const;

    Mat4 final_transformation() const;
    const MaterialID material_id() const;
    const bool is_visible() const;
    MeshID instanced_mesh_id() const;
    SubMeshID instanced_submesh_id() const;

private:
    Mesh mesh_;
};

struct StaticChunkHolder {
    typedef std::shared_ptr<StaticChunkHolder> ptr;

    std::unordered_map<GeomID, StaticChunk::ptr> chunks;
};


class OctreePartitioner :
    public Partitioner {

public:
    OctreePartitioner(Stage* ss):
        Partitioner(ss) {}

    void add_actor(ActorID obj);
    void remove_actor(ActorID obj);

    void add_geom(GeomID geom_id);
    void remove_geom(GeomID geom_id);

    void add_light(LightID obj);
    void remove_light(LightID obj);

    void add_particle_system(ParticleSystemID ps);
    void remove_particle_system(ParticleSystemID ps);

    std::vector<LightID> lights_visible_from(CameraID camera_id);
    std::vector<RenderablePtr> geometry_visible_from(CameraID camera_id);

    void event_actor_changed(ActorID ent);
private:
    Octree tree_;

    std::map<ActorID, std::vector<BoundableEntity*> > actor_to_registered_subactors_;

    std::map<ActorID, sig::connection> actor_changed_connections_;
    std::map<const BoundableEntity*, RenderablePtr> boundable_to_renderable_;
    std::map<const BoundableEntity*, LightID> boundable_to_light_;
};


}

#endif // OCTREE_PARTITIONER_H
