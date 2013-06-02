#include "octree_partitioner.h"

#include "../stage.h"
#include "../light.h"
#include "../entity.h"
#include "../camera.h"

namespace kglt {

void OctreePartitioner::event_entity_changed(ActorID ent) {
    L_DEBUG("Actor changed, updating partitioner");
    remove_entity(ent);
    add_entity(ent);
}

void OctreePartitioner::add_entity(ActorID obj) {
    L_DEBUG("Adding entity to the partitioner");

    Actor& ent = stage().entity(obj);
    for(uint16_t i = 0; i < ent.subentity_count(); ++i) {
        //All subentities are boundable
        Boundable* boundable = dynamic_cast<Boundable*>(&ent.subentity(i));        
        tree_.grow(boundable);

        entity_to_registered_subentities_[obj].push_back(boundable);
        boundable_to_subentity_[boundable] = ent._subentities().at(i);
    }

    //Connect the changed signal
    entity_changed_connections_[obj] = ent.signal_mesh_changed().connect(sigc::mem_fun(this, &OctreePartitioner::event_entity_changed));
}

void OctreePartitioner::remove_entity(ActorID obj) {
    L_DEBUG("Removing entity from the partitioner");

    //Remove all boundable subentities that were linked to the entity
    for(Boundable* boundable: entity_to_registered_subentities_[obj]) {
        tree_.shrink(boundable);
        boundable_to_subentity_.erase(boundable);
    }

    //Erase the list of subentities linked to this entity
    entity_to_registered_subentities_.erase(obj);

    //Disconnect the changed signal
    entity_changed_connections_[obj].disconnect();
    entity_changed_connections_.erase(obj);
}

void OctreePartitioner::add_light(LightID obj) {
    Light& light = stage().light(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(&light);
    assert(boundable);
    tree_.grow(boundable);
    boundable_to_light_[boundable] = obj;
}

void OctreePartitioner::remove_light(LightID obj) {
    Light& light = stage().light(obj);
    Boundable* boundable = dynamic_cast<Boundable*>(&light);
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
            if(container::contains(boundable_to_subentity_, obj)) {
                //Build a list of visible subentities
                results.push_back(boundable_to_subentity_[obj]);
            }
        }
    }

    return results;
}

std::vector<LightID> OctreePartitioner::lights_within_range(const kmVec3& location) {
    std::vector<LightID> lights;
/*
    //Go through the visible nodes
    for(OctreeNode* node: tree_.nodes_visible_from(cam.frustum())) {
        //Go through the objects
        for(const Boundable* obj: node->objects()) {
            if(container::contains(boundable_to_light_, obj)) {
                //Build a list of visible subentities
                results.push_back(boundable_to_light_[obj]);
            }
        }
    }*/

    return lights;
}

}
