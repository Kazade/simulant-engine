#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <tr1/memory>
#include <set>
#include <vector>

#include "generic/managed.h"
#include "utils/geometry_buffer.h"
#include "types.h"

namespace kglt {

class SubActor;

class Partitioner:
    public Managed<Partitioner> {

public:
    Partitioner(Stage& ss):
        stage_(ss) {}

    virtual void add_actor(ActorID obj) = 0;
    virtual void remove_actor(ActorID obj) = 0;

    virtual void add_light(LightID obj) = 0;
    virtual void remove_light(LightID obj) = 0;

    virtual std::vector<LightID> lights_within_range(const kmVec3& location) = 0;
    virtual std::vector<std::shared_ptr<SubActor>> geometry_visible_from(CameraID camera_id) = 0;

protected:
    Stage& stage() { return stage_; }

private:
    Stage& stage_;
};

}

#endif // PARTITIONER_H
