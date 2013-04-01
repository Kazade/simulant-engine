#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include <sigc++/sigc++.h>

#include "../partitioner.h"
#include "octree.h"

namespace kglt {

class SubEntity;

typedef std::tr1::shared_ptr<SubEntity> SubEntityPtr;

class OctreePartitioner :
    public Partitioner {

public:
    OctreePartitioner(SubScene& ss):
        Partitioner(ss) {}

    void add_entity(EntityID obj);
    void remove_entity(EntityID obj);

    void add_light(LightID obj);
    void remove_light(LightID obj);

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::vector<SubEntityPtr> geometry_visible_from(CameraID camera_id);

    void event_entity_changed(EntityID ent);
private:
    Octree tree_;

    std::map<EntityID, std::vector<Boundable*> > entity_to_registered_subentities_;

    std::map<EntityID, sigc::connection> entity_changed_connections_;
    std::map<const Boundable*, SubEntityPtr> boundable_to_subentity_;
    std::map<const Boundable*, LightID> boundable_to_light_;
};


}

#endif // OCTREE_PARTITIONER_H
