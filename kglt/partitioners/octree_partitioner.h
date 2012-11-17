#ifndef OCTREE_PARTITIONER_H
#define OCTREE_PARTITIONER_H

#include "partitioner.h"
#include "generic/tree.h"

#include "kazmath/aabb.h"

namespace kglt {

class OctreePartitioner :
    public Partitioner {

public:
    void add_entity(EntityID obj);
    void remove_entity(EntityID obj);

    void add_light(Light& obj);
    void remove_light(Light& obj);

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::vector<SubEntity::ptr> geometry_visible_from(CameraID camera_id, SceneGroupID scene_group_id=0);


};


}

#endif // OCTREE_PARTITIONER_H
