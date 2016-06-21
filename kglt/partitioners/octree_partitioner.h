#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include <kazbase/signals.h>

#include "../partitioner.h"
#include "../interfaces.h"
#include "../mesh.h"
#include "impl/octree.h"

namespace kglt {

class Renderable;

typedef std::shared_ptr<Renderable> RenderablePtr;

bool should_split_predicate(const impl::OctreeNode *node);
bool should_merge_predicate(const impl::Octree::NodeList& nodes);


class OctreePartitioner :
    public Partitioner {

public:
    OctreePartitioner(Stage* ss):
        Partitioner(ss),
        tree_(ss, &should_split_predicate, &should_merge_predicate) {}

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

    MeshID debug_mesh_id() override { return tree_.debug_mesh_id(); }
private:
    impl::Octree tree_;

    std::map<ActorID, std::vector<BoundableEntity*> > actor_to_registered_subactors_;

    std::map<ActorID, sig::connection> actor_changed_connections_;
    std::map<const BoundableEntity*, RenderablePtr> boundable_to_renderable_;
    std::map<const BoundableEntity*, LightID> boundable_to_light_;
};


}

#endif // OCTREE_PARTITIONER_H
