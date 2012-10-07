#ifndef NULL_PARTITIONER_H
#define NULL_PARTITIONER_H

#include "../mesh.h"
#include "../light.h"

#include "../partitioner.h"

namespace kglt {

class NullPartitioner : public Partitioner {
public:
    NullPartitioner(Scene& scene):
        Partitioner(scene) {}

    void add(Mesh& obj) {
        all_meshes_.insert(obj.id());
    }

    void remove(Mesh& obj) {
        all_meshes_.erase(obj.id());
    }

    void relocate(Mesh& obj) {}

    void add(Light& obj) {
        all_lights_.insert(obj.id());
    }

    void remove(Light& obj) {
        all_lights_.erase(obj.id());
    }

    void relocate(Light& obj) {}

    std::vector<LightID> lights_within_range(const kmVec3& location);
    std::set<MeshID> meshes_visible_from(CameraID camera_id, SceneGroupID scene_group_id=0);

private:
    std::set<MeshID> all_meshes_;
    std::set<LightID> all_lights_;
};

}

#endif // NULL_PARTITIONER_H
