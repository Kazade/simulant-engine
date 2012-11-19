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

std::vector<kmAABB> OctreeNode::calculate_child_bounds(float child_width) {
    std::vector<kmAABB> result;
    result.resize(8);

    kmVec3 child_centre;
    kmScalar quarter_strict = strict_diameter() / 4;

    //Check NEGX_NEGY_NEGZ
    kmVec3Fill(&child_centre, centre().x - quarter_strict, centre().y - quarter_strict, centre().z - quarter_strict);
    kmAABBInitialize(
        &result[NEGX_NEGY_NEGZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check POSX_NEGY_NEGZ
    kmVec3Fill(&child_centre, centre().x + quarter_strict, centre().y - quarter_strict, centre().z - quarter_strict);
    kmAABBInitialize(
        &result[POSX_NEGY_NEGZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check NEGX_POSY_NEGZ
    kmVec3Fill(&child_centre, centre().x - quarter_strict, centre().y + quarter_strict, centre().z - quarter_strict);
    kmAABBInitialize(
        &result[NEGX_POSY_NEGZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check POSX_POSY_NEGZ
    kmVec3Fill(&child_centre, centre().x + quarter_strict, centre().y + quarter_strict, centre().z - quarter_strict);
    kmAABBInitialize(
        &result[POSX_POSY_NEGZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check NEGX_NEGY_POSZ
    kmVec3Fill(&child_centre, centre().x - quarter_strict, centre().y - quarter_strict, centre().z + quarter_strict);
    kmAABBInitialize(
        &result[NEGX_NEGY_POSZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check POSX_NEGY_POSZ
    kmVec3Fill(&child_centre, centre().x + quarter_strict, centre().y - quarter_strict, centre().z + quarter_strict);
    kmAABBInitialize(
        &result[POSX_NEGY_POSZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check NEGX_POSY_POSZ
    kmVec3Fill(&child_centre, centre().x - quarter_strict, centre().y + quarter_strict, centre().z + quarter_strict);
    kmAABBInitialize(
        &result[NEGX_POSY_POSZ],
        &child_centre,
        child_width, child_width, child_width
    );

    //Check POSX_POSY_POSZ
    kmVec3Fill(&child_centre, centre().x + quarter_strict, centre().y + quarter_strict, centre().z + quarter_strict);
    kmAABBInitialize(
        &result[POSX_POSY_POSZ],
        &child_centre,
        child_width, child_width, child_width
    );

    return result;
}

std::vector<kmAABB> OctreeNode::calculate_child_loose_bounds() {
    return calculate_child_bounds(this->strict_diameter());
}

std::vector<kmAABB> OctreeNode::calculate_child_strict_bounds() {
    return calculate_child_bounds(this->strict_diameter() / 2.0);
}

/**
 * @brief OctreeNode::insert_into_subtree
 * @param obj - The object to insert
 * @return The final OctreeNode that the object was inserted into
 *
 * This method recursively searches down the tree to find the lowest
 * node that can fit the object. If the object doesn't fit inside any
 * nodes then an ObjectDoesNotFitError() is thrown.
 */
OctreeNode& OctreeNode::insert_into_subtree(const Boundable* obj) {
    /*
     *  Optimizations:
     *
     *  1. the calculate_child_strict_bounds call does unnecessary work by creating bounds for all
     *     children, rather than the few that we need
     *  2. There's a lot of passing vectors by value rather than reference
     */

    kmAABB loose = absolute_loose_bounds();
    kmAABB obj_bounds = obj->absolute_bounds();

    std::vector<OctreePosition> destination_children;

    if(kmAABBContainsAABB(&loose, &obj_bounds) == KM_CONTAINS_ALL) {
        //If this node fully contains the object, work out if it fits any of the children
        std::vector<kmAABB> child_bounds = this->calculate_child_loose_bounds();

        for(uint8_t i = 0; i < child_bounds.size(); ++i) {
            if(kmAABBContainsAABB(&child_bounds[i], &obj_bounds) == KM_CONTAINS_ALL) {
                destination_children.push_back((OctreePosition) i);
            }
        }

    } else {
        //This should never really happen unless the function has been called outside of Octree::grow()
        throw ObjectDoesNotFitError(obj);
    }

    if(destination_children.empty()) {
        //Add to this node
        this->add_object(obj);
        return *this;
    } else if (destination_children.size() == 1) {
        OctreeNode& child = create_child(destination_children[0]);
        return child.insert_into_subtree(obj);
    } else {
        //Work out where the object's centre point is
        std::vector<kmAABB> child_bounds = this->calculate_child_strict_bounds();
        kmVec3 obj_centre = obj->centre();
        for(OctreePosition pos: destination_children) {
            if(kmAABBContainsPoint(&child_bounds[pos], &obj_centre)) {
                OctreeNode& child = create_child(pos);
                return child.insert_into_subtree(obj);
            }
        }

        throw std::runtime_error("Something went seriously wrong while inserting an object into the octree.");
    }
}


OctreeNode::OctreeNode(OctreeNode* parent, float strict_diameter, const kmVec3& centre):
    parent_(parent),
    centre_(centre) {

    kmAABBInitialize(&strict_bounds_, &centre_, strict_diameter, strict_diameter, strict_diameter);
    kmAABBInitialize(&loose_bounds_, &centre_, strict_diameter * 2, strict_diameter * 2, strict_diameter * 2);
}

