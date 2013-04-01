#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <tr1/memory>
#include <set>
#include <vector>

#include "generic/managed.h"
#include "utils/geometry_buffer.h"
#include "types.h"

namespace kglt {

class SubEntity;

class Partitioner:
    public Managed<Partitioner> {

public:
    Partitioner(SubScene& ss):
        subscene_(ss) {}

    virtual void add_entity(EntityID obj) = 0;
    virtual void remove_entity(EntityID obj) = 0;

    virtual void add_light(LightID obj) = 0;
    virtual void remove_light(LightID obj) = 0;

    virtual std::vector<LightID> lights_within_range(const kmVec3& location) = 0;
    virtual std::vector<std::tr1::shared_ptr<SubEntity>> geometry_visible_from(CameraID camera_id) = 0;

protected:
    SubScene& subscene() { return subscene_; }

private:
    SubScene& subscene_;
};

}

#endif // PARTITIONER_H
