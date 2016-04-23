#include <kazmath/aabb3.h>

#include "octree.h"

#include <kazbase/logging.h>
#include <kazbase/list_utils.h>
#include "../frustum.h"

namespace kglt {

Octree::Octree() {

}

OctreeNode& Octree::find(const BoundableEntity* object) {
    if(!object_node_lookup_.count(object)) {
        throw std::logic_error("Object does not exist in the tree");
    }

    return *object_node_lookup_.at(object);
}

void visible_node_finder(OctreeNode* self, std::vector<OctreeNode*>& result, const Frustum& frustum) {

    //Returns > 0 if it's even partially contained (see FrustumClassification)
    if(frustum.intersects_aabb(self->absolute_loose_bounds())) {
        result.push_back(self);
    }

    //Go through the child nodes (if any) and check those
    for(uint8_t i = 0; i < 8; ++i) {
        if(self->has_child((OctreePosition)i)) {
            visible_node_finder(&self->child((OctreePosition)i), result, frustum);
        }
    }
}

std::vector<OctreeNode*> Octree::nodes_visible_from(const Frustum& frustum) {
    std::vector<OctreeNode*> result;

    //Find all the nodes that are within the frustum
    visible_node_finder(&root(), result, frustum);

    return result;
}

void Octree::shrink(const BoundableEntity* object, ShrinkCallback callback) {
    assert(object);

    if(!container::contains(this->object_node_lookup_, object)) {
        throw std::logic_error("Tried to remove an object that doesn't exist in the tree");
    }

    OctreeNode& node = *container::const_get(this->object_node_lookup_, object);
    node.remove_object(object);

    if(!node.has_objects() && !node.child_count()) {
        L_WARN("FIXME: Node should be deleted but this is not yet implemented");
    }

    _unregister_object(object);
}

void Octree::grow(const BoundableEntity *object, GrowCallback callback) {
    assert(object);

    AABB obj_bounds = object->transformed_aabb();
    float obj_diameter = object->diameter();

    if(obj_diameter < kmEpsilon) {
        L_DEBUG("Not adding object to the octree because it has no volume");
        return;
    }

    if(!root_) {
        //L_DEBUG("Creating root node");
        /*
         *  We don't have a root node yet, so create one centred around
         *  the object with strict bounds that encompass it.
         */
        float node_size = obj_diameter;

        root_.reset(new OctreeNode(
            nullptr,
            node_size,
            object->centre()
        ));

        //L_DEBUG(_u("Root node created with strict width of: {0}").format(node_size));
    }

    //While the object is too big for the root
    while(kmAABB3ContainsAABB(&root().absolute_strict_bounds(), &obj_bounds) != KM_CONTAINS_ALL) {
        //L_DEBUG("Root node cannot contain object, growing upwards");

        /*
         * 1. Find centre point of parent (we do this by picking the nearest corner of the current root node to the
         * object's central point).
         * 2. Create a new node with the central point and twice the strict diameter of the current root
         * 3. Make the current root node the appropriate child of the new root (we know which by using the chosen
         * corner point)
         * 4. Make the new node the root
         */

        kmVec3 new_centre, obj_centre;
        kmAABB3Centre(&obj_bounds, &obj_centre);
        kmVec3Assign(&new_centre, &root().centre());
        float half_current = root().strict_diameter() / 2;
        new_centre.x += (obj_centre.x < root().centre().x) ? -half_current : half_current;
        new_centre.y += (obj_centre.y < root().centre().y) ? -half_current : half_current;
        new_centre.z += (obj_centre.z < root().centre().z) ? -half_current : half_current;

        OctreeNode::ptr new_root(new OctreeNode(nullptr, root().strict_diameter() * 2, new_centre));


        /*
         * Which child of the new root is the current root
         * to become?
         */
        OctreePosition root_to_become_child;

        if(new_centre.x < root().centre().x) {
            if(new_centre.y < root().centre().y) {
                if(new_centre.z < root().centre().z) {
                    root_to_become_child = POSX_POSY_POSZ;
                } else {
                    root_to_become_child = POSX_POSY_NEGZ;
                }
            } else {
                if(new_centre.z < root().centre().z) {
                    root_to_become_child = POSX_NEGY_POSZ;
                } else {
                    root_to_become_child = POSX_NEGY_NEGZ;
                }
            }
        } else {
            if(new_centre.y < root().centre().y) {
                if(new_centre.z < root().centre().z) {
                    root_to_become_child = NEGX_POSY_POSZ;
                } else {
                    root_to_become_child = NEGX_POSY_NEGZ;
                }
            } else {
                if(new_centre.z < root().centre().z) {
                    root_to_become_child = NEGX_NEGY_POSZ;
                } else {
                    root_to_become_child = NEGX_NEGY_NEGZ;
                }
            }
        }

        OctreeNode::ptr old_root = root_;

        new_root->children_[root_to_become_child] = old_root;
        old_root->parent_ = new_root.get();
        root_ = new_root;
    }

    //Now insert into the subtree
    this->_register_object(&root().insert_into_subtree(object, callback), object);
}

kmAABB3 OctreeNode::calculate_child_bounds(OctreePosition pos, float child_width) {
    kmAABB3 result;
    kmVec3 child_centre;
    kmScalar quarter_strict = strict_diameter() / 4;

    switch(pos) {
    case NEGX_NEGY_NEGZ: {
        //Check NEGX_NEGY_NEGZ
        kmVec3Fill(
            &child_centre,
            centre().x - quarter_strict,
            centre().y - quarter_strict,
            centre().z - quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case POSX_NEGY_NEGZ: {
        //Check POSX_NEGY_NEGZ
        kmVec3Fill(
            &child_centre,
            centre().x + quarter_strict,
            centre().y - quarter_strict,
            centre().z - quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case NEGX_POSY_NEGZ: {
        //Check NEGX_POSY_NEGZ
        kmVec3Fill(
            &child_centre,
            centre().x - quarter_strict,
            centre().y + quarter_strict,
            centre().z - quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case POSX_POSY_NEGZ: {
        //Check POSX_POSY_NEGZ
        kmVec3Fill(
            &child_centre,
            centre().x + quarter_strict,
            centre().y + quarter_strict,
            centre().z - quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case NEGX_NEGY_POSZ: {
        //Check NEGX_NEGY_POSZ
        kmVec3Fill(
            &child_centre,
            centre().x - quarter_strict,
            centre().y - quarter_strict,
            centre().z + quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case POSX_NEGY_POSZ: {
        //Check POSX_NEGY_POSZ
        kmVec3Fill(
            &child_centre,
            centre().x + quarter_strict,
            centre().y - quarter_strict,
            centre().z + quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case NEGX_POSY_POSZ: {
        //Check NEGX_POSY_POSZ
        kmVec3Fill(
            &child_centre,
            centre().x - quarter_strict,
            centre().y + quarter_strict,
            centre().z + quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    case POSX_POSY_POSZ: {
        //Check POSX_POSY_POSZ
        kmVec3Fill(
            &child_centre,
            centre().x + quarter_strict,
            centre().y + quarter_strict,
            centre().z + quarter_strict
        );
        kmAABB3Initialize(
            &result,
            &child_centre,
            child_width, child_width, child_width
        );
    } break;
    default:
        throw std::logic_error("Invalid option");
    }

    return result;
}

OctreeNode& OctreeNode::create_child(OctreePosition pos) {
    if(has_child(pos)) {
        return child(pos);
    }

    kmAABB3 bounds = calculate_child_strict_bounds(pos);
    kmVec3 centre;
    kmAABB3Centre(&bounds, &centre);

    children_[pos].reset(new OctreeNode(this, kmAABB3DiameterX(&bounds), centre));

    return *children_[pos];
}

kmAABB3 OctreeNode::calculate_child_loose_bounds(OctreePosition pos) {
    return calculate_child_bounds(pos, this->strict_diameter());
}

kmAABB3 OctreeNode::calculate_child_strict_bounds(OctreePosition pos) {
    return calculate_child_bounds(pos, this->strict_diameter() / 2.0);
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
OctreeNode& OctreeNode::insert_into_subtree(const BoundableEntity* obj, GrowCallback callback) {
    AABB obj_bounds = obj->transformed_aabb();
    float obj_diameter = obj->diameter();

    if(obj_diameter < this->strict_diameter() / 2) {
        //Object will fit into child
        kmVec3 centre = obj->centre();

        //L_DEBUG("Object will fit into child, traversing next level");
        for(uint8_t i = 0; i < 8; ++i) {
            kmAABB3 bounds = calculate_child_strict_bounds((OctreePosition)i);

            if(kmAABB3ContainsPoint(&bounds, &centre)) {
                OctreeNode& child = create_child((OctreePosition) i);
                return child.insert_into_subtree(obj, callback);
            }
        }

        throw std::logic_error("Something went wrong while adding the object to the Octree");
    } else {
        //L_DEBUG("Destination node for object found");

        // Only insert if there is no grow callback, or it returns true
        if(!callback || callback(obj, this)) {
            //Add to this node
            this->add_object(obj);
        }

        // Note, the object might not have been inserted into this node if the callback
        // returned false
        return *this;
    }
}

OctreeNode::OctreeNode(OctreeNode* parent, float strict_diameter, const kmVec3& centre):
    parent_(parent),
    centre_(centre) {

    kmAABB3Initialize(&strict_bounds_, &centre_, strict_diameter, strict_diameter, strict_diameter);
    kmAABB3Initialize(&loose_bounds_, &centre_, strict_diameter * 2, strict_diameter * 2, strict_diameter * 2);
}

}
