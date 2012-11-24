
#include "octree_partitioner.h"
#include "../scene.h"

namespace kglt {


void OctreePartitioner::add_entity(EntityID obj) {
    Entity& ent = scene().entity(obj);
    for(uint16_t i = 0; i < ent.subentity_count(); ++i) {
        //All subentities are boundable
        Boundable* boundable = dynamic_cast<Boundable*>(&ent.subentity(i));
        tree_.grow(boundable);
        boundable_to_subentity_[boundable] = ent._subentities().at(i);
    }
}

void OctreePartitioner::remove_entity(EntityID obj) {
    //tree_.shrink(&scene().entity(obj));
}

void OctreePartitioner::add_light(LightID obj) {
    Light& light = scene().entity(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(&light);
    assert(boundable);
    tree_.grow(boundable);
    boundable_to_light_[boundable] = obj;
}

void OctreePartitioner::remove_light(LightID &obj) {
    Light& light = scene().entity(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(&light);
    assert(boundable);
    tree_.shrink(boundable);
    boundable_to_light_.erase(boundable);
}

std::vector<SubEntity::ptr> OctreePartitioner::geometry_visible_from(CameraID camera_id, SceneGroupID scene_group_id) {
    std::vector<SubEntity::ptr> results;

    Camera& cam = scene().camera(camera_id);

    /**
     *  FIXME: A tree_->objects_visible_from(cam.frustum()); would be faster
     */

    //Go through the visible nodes
    for(OctreeNode* node: tree_.nodes_visible_from(cam.frustum())) {
        //Go through the objects
        for(const Boundable* obj: node->objects()) {
            if(container::contains(boundable_to_subentity_, obj)) {
                //Build a list of visible subentities
                results.push_back(boundable_to_subentity_[obj]);
            }
        }
    }

    return results;
}

std::vector<LightID> OctreePartitioner::lights_within_range(const kmVec3& location) {
    /*
     *  FIXME: Need to think about lights!
     */

    std::vector<LightID> lights;
    return lights;
}

}
