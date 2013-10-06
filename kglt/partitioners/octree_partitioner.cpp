#include "octree_partitioner.h"

#include "../stage.h"
#include "../light.h"
#include "../actor.h"
#include "../camera.h"

namespace kglt {

void OctreePartitioner::event_actor_changed(ActorID ent) {
    L_DEBUG("Actor changed, updating partitioner");
    remove_actor(ent);
    add_actor(ent);
}

void OctreePartitioner::add_actor(ActorID obj) {
    L_DEBUG("Adding actor to the partitioner");

    auto ent = stage().actor(obj);
    for(uint16_t i = 0; i < ent->subactor_count(); ++i) {
        //All subactors are boundable
        Boundable* boundable = dynamic_cast<Boundable*>(&ent->subactor(i));
        tree_.grow(boundable);

        actor_to_registered_subactors_[obj].push_back(boundable);
        boundable_to_subactor_[boundable] = ent->_subactors().at(i);
    }

    //Connect the changed signal
    actor_changed_connections_[obj] = ent->signal_mesh_changed().connect(std::bind(&OctreePartitioner::event_actor_changed, this, std::placeholders::_1));
}

void OctreePartitioner::remove_actor(ActorID obj) {
    L_DEBUG("Removing actor from the partitioner");

    //Remove all boundable subactors that were linked to the actor
    for(Boundable* boundable: actor_to_registered_subactors_[obj]) {
        tree_.shrink(boundable);
        boundable_to_subactor_.erase(boundable);
    }

    //Erase the list of subactors linked to this actor
    actor_to_registered_subactors_.erase(obj);

    //Disconnect the changed signal
    actor_changed_connections_[obj].disconnect();
    actor_changed_connections_.erase(obj);
}

void OctreePartitioner::add_light(LightID obj) {
    //FIXME: THis is nasty and dangerous
    auto light = stage().light(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(light.__object.get());
    assert(boundable);
    tree_.grow(boundable);
    boundable_to_light_[boundable] = obj;
}

void OctreePartitioner::remove_light(LightID obj) {
    auto light = stage().light(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(light.__object.get());
    assert(boundable);
    tree_.shrink(boundable);
    boundable_to_light_.erase(boundable);
}

std::vector<SubActor::ptr> OctreePartitioner::geometry_visible_from(CameraID camera_id) {
    std::vector<SubActor::ptr> results;

    //If the tree has no root then we return nothing
    if(!tree_.has_root()) {
        return results;
    }

    Camera& cam = stage().scene().camera(camera_id);

    /**
     *  FIXME: A tree_->objects_visible_from(cam.frustum()); would be faster
     */

    //Go through the visible nodes
    for(OctreeNode* node: tree_.nodes_visible_from(cam.frustum())) {
        //Go through the objects
        for(const Boundable* obj: node->objects()) {
            if(container::contains(boundable_to_subactor_, obj)) {
                //Build a list of visible subactors
                results.push_back(boundable_to_subactor_[obj]);
            }
        }
    }

    return results;
}

std::vector<LightID> OctreePartitioner::lights_within_range(const Vec3& location) {
    std::vector<LightID> lights;
/*
    //Go through the visible nodes
    for(OctreeNode* node: tree_.nodes_visible_from(cam.frustum())) {
        //Go through the objects
        for(const Boundable* obj: node->objects()) {
            if(container::contains(boundable_to_light_, obj)) {
                //Build a list of visible subactors
                results.push_back(boundable_to_light_[obj]);
            }
        }
    }*/

    return lights;
}

}
