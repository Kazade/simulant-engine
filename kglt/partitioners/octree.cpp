#include "kazbase/logging/logging.h"
#include "kazbase/list_utils.h"
#include "octree.h"

Octree::Octree() {

}

OctreeNode& Octree::find(const Boundable* object) {
    if(!container::contains(object_node_lookup_, object)) {
        throw std::logic_error("Object does not exist in the tree");
    }

    return *container::const_get(object_node_lookup_, object);
}

void Octree::grow(const Boundable *object) {
    assert(object);

    kmAABB obj_bounds = object->absolute_bounds();

    if(!root_) {
        L_DEBUG("Creating root node");
        /*
         *  We don't have a root node yet, so create one centred around
         *  the object with strict bounds that encompass it.
         */
        float node_size = std::ceil(std::max(
            kmAABBDiameterX(&obj_bounds),
            std::max(
                kmAABBDiameterY(&obj_bounds),
                kmAABBDiameterZ(&obj_bounds)
            )
        ));

        root_.reset(new OctreeNode(
            nullptr,
            node_size,
            object->centre()
        ));

        L_DEBUG("Root node created with strict width of: " + boost::lexical_cast<std::string>(node_size));
    }

    //While the object is outside the root (this won't be entered if the root has just been created)
    while(kmAABBContainsAABB(&root().absolute_loose_bounds(), &obj_bounds) != KM_CONTAINS_ALL) {
        L_DEBUG("Root node cannot contain object, growing upwards");
        assert(0 && "Not implemented");
    }

    //Now insert into the subtree
    root().insert_into_subtree(object);
}


OctreeNode::OctreeNode(OctreeNode* parent, float strict_diameter, const kmVec3& centre):
    parent_(parent),
    centre_(centre) {

    kmAABBInitialize(&strict_bounds_, &centre_, strict_diameter, strict_diameter, strict_diameter);
    kmAABBInitialize(&loose_bounds_, &centre_, strict_diameter * 2, strict_diameter * 2, strict_diameter * 2);
}

