#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include "../partitioner.h"
#include "octree.h"

#include <kazbase/signals.h>

namespace kglt {

class Renderable;

typedef std::shared_ptr<Renderable> RenderablePtr;

class OctreePartitioner :
    public Partitioner {

public:
    OctreePartitioner(Stage& ss):
        Partitioner(ss) {}

    void add_actor(ActorID obj);
    void remove_actor(ActorID obj);

    void add_light(LightID obj);
    void remove_light(LightID obj);

    void add_particle_system(ParticleSystemID ps);
    void remove_particle_system(ParticleSystemID ps);

    std::vector<LightID> lights_within_range(const Vec3& location);
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
