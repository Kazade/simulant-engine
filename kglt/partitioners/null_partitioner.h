#ifndef NULL_PARTITIONER_H
#define NULL_PARTITIONER_H

#include "../entity.h"
#include "../light.h"

#include "../partitioner.h"

namespace kglt {

class NullPartitioner : public Partitioner {
public:
    NullPartitioner(Scene& scene):
        Partitioner(scene) {}

    void add_entity(EntityID obj) {
        all_entities_.insert(obj);
    }

    void remove_entity(EntityID obj) {
        all_entities_.erase(obj);
    }

    void add_light(LightID obj) {
        all_lights_.insert(obj);
    }

    void remove_light(LightID obj) {
        all_lights_.erase(obj);
    }

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::vector<SubEntity::ptr> geometry_visible_from(CameraID camera_id, SceneGroupID scene_group_id=0);

private:
    std::set<EntityID> all_entities_;
    std::set<LightID> all_lights_;
};

}

#endif // NULL_PARTITIONER_H
