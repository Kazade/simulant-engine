#ifndef NULL_PARTITIONER_H
#define NULL_PARTITIONER_H

#include "../partitioner.h"

namespace kglt {

class SubActor;

class NullPartitioner : public Partitioner {
public:
    NullPartitioner(Stage& ss):
        Partitioner(ss) {}

    void add_actor(ActorID obj) {
        all_actors_.insert(obj);
    }

    void remove_actor(ActorID obj) {
        all_actors_.erase(obj);
    }

    void add_light(LightID obj) {
        all_lights_.insert(obj);
    }

    void remove_light(LightID obj) {
        all_lights_.erase(obj);
    }

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::vector<std::shared_ptr<SubActor>> geometry_visible_from(CameraID camera_id);

private:
    std::set<ActorID> all_actors_;
    std::set<LightID> all_lights_;
};

}

#endif // NULL_PARTITIONER_H
