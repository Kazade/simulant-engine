#ifndef OCTREE_H
#define OCTREE_H

#include <set>
#include <map>
#include <stdexcept>
#include <vector>
#include <memory>
#include <kazmath/kazmath.h>

#include "../generic/data_carrier.h"
#include "../generic/managed.h"
#include "../interfaces.h"
#include "../types.h"

#include <kazbase/list_utils.h>
/*
 * BANTER FOLLOWS!
 *
 * So, Octrees are cool. Our one needs to be dynamic and loose. This means that:
 *
 * 1. If an object is added outside the root node, we need to adjust for it. The steps in this case are:
 *
 * while new_object.outside(root):
 *  new_root = OctreeNode(width=root.width*2, position=root.position - direction_of_object)
 *  new_root.add_child(root)
 *  root = new_root
 * Basically, we dynamically grow upwards towards the object.
 *
 * 2. Each node of the octree should be loose. Which means that the bounds of the node are half the size
 * of the parent, rather than a quarter of the size.
 */

namespace kglt {

enum OctreePosition {
    NEGX_POSY_NEGZ = 0,
    POSX_POSY_NEGZ,
    POSX_POSY_POSZ,
    NEGX_POSY_POSZ,
    NEGX_NEGY_NEGZ,
    POSX_NEGY_NEGZ,
    POSX_NEGY_POSZ,
    NEGX_NEGY_POSZ
};

class ChildNodeDoesNotExist :
    public std::logic_error {

    ChildNodeDoesNotExist():
        std::logic_error("Attempted to get a child node that doesn't exist") {}
};


class ObjectDoesNotFitError :
    public std::runtime_error {

public:

    ObjectDoesNotFitError(const Boundable* obj):
        std::runtime_error("Object does not fit into the octree node"),
        object(obj) {

    }

    const Boundable* object;
};


class Octree;
class OctreeNode;

typedef std::function<bool (const BoundableEntity*, OctreeNode*)> GrowCallback;
typedef std::function<bool (const BoundableEntity*, OctreeNode*)> ShrinkCallback;

class OctreeNode :
    public Managed<OctreeNode>,
    public generic::DataCarrier {

public:
    OctreeNode(OctreeNode* parent, float strict_diameter, const kmVec3 &centre);

    const kmVec3& centre() const {
        return centre_;
    }

    float width() const;
    float loose_width() const;

    uint8_t child_count() const { return children_.size(); }
    uint32_t object_count() const;

    OctreeNode& child(OctreePosition pos) {
        return *children_.at(pos);
    }

    bool has_child(OctreePosition pos) const {
        return children_.count(pos);
    }
    bool has_objects() const { return !objects_.empty(); }

    bool is_root() const { return !parent_; }

    const kmAABB3& absolute_loose_bounds() const { return loose_bounds_; }
    const kmAABB3& absolute_strict_bounds() const { return strict_bounds_; }

    const float loose_diameter() const {
        //Any dimension will do...
        return kmAABB3DiameterX(&loose_bounds_);
    }
    const float strict_diameter() const {
        //Any dimension will do...
        return kmAABB3DiameterX(&strict_bounds_);
    }

    const std::set<const BoundableEntity*>& objects() const { return objects_; }
private:
    OctreeNode* parent_;
    std::map<OctreePosition, std::shared_ptr<OctreeNode> > children_;
    std::set<const BoundableEntity*> objects_;

    AABB strict_bounds_;
    AABB loose_bounds_;
    Vec3 centre_;

    OctreeNode& create_child(OctreePosition pos);

    OctreeNode& insert_into_subtree(const BoundableEntity *obj, GrowCallback callback);

    void add_object(const BoundableEntity* obj) {
        objects_.insert(obj);
    }

    void remove_object(const BoundableEntity* obj) {
        objects_.erase(obj);
    }

    kmAABB3 calculate_child_loose_bounds(OctreePosition pos);
    kmAABB3 calculate_child_strict_bounds(OctreePosition pos);
    kmAABB3 calculate_child_bounds(OctreePosition pos, float child_width);

    friend class Octree;

};


/* A dynamic, loose Octree implementation. Things to note:
 *
 * * The tree can grow upwards and downwards as needed. Calling grow(Boundable*) will determine
 *   if nodes need to be added above or below the current tree
 * * In the event that an object exists in the cross-over between nodes, the one containing the
 *   centre point within it's "strict" bounds will take it.
 * * When an object changes location and moves outside its node, the tree will be recursed upwards
 *   until a parent is found that contains the object, and then recurse downwards to find the new
 *   node. If the object moves outside the root node this will require the tree to grow upwards. It
 *   is slower to move an object a great distance than a small one.
 * * Objects will be stored as far down the tree as they will fit
 * * If an object is added outside the bounds of the root node, the tree will grow in the direction
 *   of the new object until the root node encompasses it. This means that the Octree can move around in
 *   space. For example a fleet of spaceships moving a great distance will cause the root node to grow
 *   large, and then as child nodes empty shift in the direction of the fleet.
 * * If a node has no objects, and no children, it is removed. If the root node has no objects and only
 *   one child, then the child becomes the new root.
 */

class Octree {
public:
    Octree();

    OctreeNode& root() {
        if(!root_) {
            throw std::logic_error("Octree has not been initialized");
        }
        return *root_;
    }

    uint32_t node_count() const;

    bool has_root() const { return root_ != nullptr; }

    void grow(const BoundableEntity* object, GrowCallback callback=GrowCallback());
    void shrink(const BoundableEntity* object, ShrinkCallback callback=ShrinkCallback());
    void relocate(const BoundableEntity* object);

    OctreeNode& find(const BoundableEntity *object);

    std::vector<OctreeNode*> nodes_visible_from(const Frustum& frustum);

private:
    OctreeNode::ptr root_;
    uint32_t node_count_;

    void _increment_node_count();
    void _decrement_node_count();

    void _register_object(OctreeNode* node, const BoundableEntity* obj) {
        object_node_lookup_[obj] = node;
    }

    void _unregister_object(const BoundableEntity* obj) {
        object_node_lookup_.erase(obj);
    }

    std::map<const BoundableEntity*, OctreeNode*> object_node_lookup_;

    friend class OctreeNode;
};

}
#endif // OCTREE_H
