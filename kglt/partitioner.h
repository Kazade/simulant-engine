#ifndef PARTITIONER_H
#define PARTITIONER_H

#include <tr1/memory>
#include <set>
#include <vector>

#include "types.h"

namespace kglt {

class Partitioner {
public:
    typedef std::tr1::shared_ptr<Partitioner> ptr;

    Partitioner(Scene& scene):
        scene_(scene) {}

    virtual void add(Mesh& obj) = 0;
    virtual void remove(Mesh& obj) = 0;
    virtual void relocate(Mesh& obj) = 0;

    virtual void add(Light& obj) = 0;
    virtual void remove(Light& obj) = 0;
    virtual void relocate(Light& obj) = 0;

    virtual std::vector<LightID> lights_within_range(const kmVec3& location) = 0;
    virtual std::set<MeshID> meshes_visible_from(const Camera& camera) = 0;

protected:
    Scene& scene() { return scene_; }

private:
    Scene& scene_;
};

}

#endif // PARTITIONER_H
