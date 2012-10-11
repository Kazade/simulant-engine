#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <tr1/memory>
#include <set>
#include <vector>

#include "utils/geometry_buffer.h"
#include "types.h"

namespace kglt {

class Partitioner {
public:
    typedef std::tr1::shared_ptr<Partitioner> ptr;

    Partitioner(Scene& scene):
        scene_(scene) {}

    virtual void add_mesh(Mesh& obj) = 0;
    virtual void remove_mesh(Mesh& obj) = 0;
    virtual void relocate(Mesh& obj) = 0;

    virtual void add_light(Light& obj) = 0;
    virtual void remove_light(Light& obj) = 0;
    virtual void relocate(Light& obj) = 0;

    virtual std::vector<LightID> lights_within_range(const kmVec3& location) = 0;
    virtual std::vector<GeometryBuffer::ptr> geometry_visible_from(CameraID camera_id, SceneGroupID scene_group_id=0) = 0;

protected:
    Scene& scene() { return scene_; }

private:
    Scene& scene_;
};

}

#endif // PARTITIONER_H
