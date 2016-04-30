#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <memory>
#include <set>
#include <vector>

#include "generic/property.h"
#include "generic/managed.h"
#include "renderers/renderer.h"
#include "utils/geometry_buffer.h"
#include "types.h"
#include "interfaces.h"

namespace kglt {

class SubActor;

class Partitioner:
    public Managed<Partitioner> {

public:
    Partitioner(Stage* ss):
        stage_(ss) {}

    virtual void add_particle_system(ParticleSystemID ps) = 0;
    virtual void remove_particle_system(ParticleSystemID ps) = 0;

    virtual void add_geom(GeomID geom_id) = 0;
    virtual void remove_geom(GeomID geom_id) = 0;

    virtual void add_actor(ActorID obj) = 0;
    virtual void remove_actor(ActorID obj) = 0;

    virtual void add_light(LightID obj) = 0;
    virtual void remove_light(LightID obj) = 0;

    virtual std::vector<LightID> lights_visible_from(CameraID camera_id) = 0;
    virtual std::vector<std::shared_ptr<Renderable>> geometry_visible_from(CameraID camera_id) = 0;

protected:
    Property<Partitioner, Stage> stage = { this, &Partitioner::stage_ };

private:
    Stage* stage_;
};

}

#endif // PARTITIONER_H
