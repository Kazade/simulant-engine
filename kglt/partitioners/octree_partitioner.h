#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include <sigc++/sigc++.h>

#include "../partitioner.h"
#include "octree.h"

namespace kglt {

class SubActor;

typedef std::shared_ptr<SubActor> SubActorPtr;

class OctreePartitioner :
    public Partitioner {

public:
    OctreePartitioner(Stage& ss):
        Partitioner(ss) {}

    void add_actor(ActorID obj);
    void remove_actor(ActorID obj);

    void add_light(LightID obj);
    void remove_light(LightID obj);

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::vector<SubActorPtr> geometry_visible_from(CameraID camera_id);

    void event_actor_changed(ActorID ent);
private:
    Octree tree_;

    std::map<ActorID, std::vector<Boundable*> > actor_to_registered_subentities_;

    std::map<ActorID, sigc::connection> actor_changed_connections_;
    std::map<const Boundable*, SubActorPtr> boundable_to_subactor_;
    std::map<const Boundable*, LightID> boundable_to_light_;
};


}

#endif // OCTREE_PARTITIONER_H
