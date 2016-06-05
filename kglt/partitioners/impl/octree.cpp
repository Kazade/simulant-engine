#include "octree.h"
#include "../../types.h"
#include "../../stage.h"
#include "../../actor.h"
#include "../../light.h"

namespace kglt {
namespace impl {

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

OctreeNode::OctreeNode(Octree* octree, NodeLevel level, const Vec3& centre):
    octree_(octree),
    level_(level),
    centre_(centre) {

}

bool OctreeNode::has_children() const {
    /*
     * We keep pointers to the octree children for performance
     * if any of them are not null then this node has children
     */
    for(auto child: children_) {
        if(child) return true;
    }

    return false;
}

OctreeNode::NodeList OctreeNode::children() const {
    NodeList ret;
    for(uint8_t i = 0; i < 8; ++i) {
        if(children_[i]) {
            ret.push_back(children_[i]);
        }
    }
    return ret;
}

const float OctreeNode::diameter() const {
    return octree_->node_diameter(this);
}


Octree::Octree(StagePtr stage):
    stage_(stage) {

}

Octree::VectorHash Octree::generate_vector_hash(const Vec3& vec) {
    auto round_float = [](float x) {
        // FIXME: There still might be edge cases here... we need floats which are
        // reasonably close together to have the same hash value.
        // Round to 2 decimal places
        return std::roundf(x * 100.0f) / 100.0f;
    };

    float x = round_float(vec.x);
    float y = round_float(vec.y);
    float z = round_float(vec.z);

    std::size_t seed = 0;

    std::hash_combine(seed, x);
    std::hash_combine(seed, y);
    std::hash_combine(seed, z);

    return Octree::VectorHash(seed);
}

OctreeNode* Octree::insert_actor(ActorID actor_id) {
    auto actor = stage_->actor(actor_id);
    OctreeNode* node = get_or_create_node(actor.get());
    node->data->actor_ids_.insert(actor_id);
    actor_lookup_.insert(std::make_pair(actor_id, node));
    return node;
}

void Octree::remove_actor(ActorID actor_id) {
    auto node = locate_actor(actor_id);

    node->data->actor_ids_.erase(actor_id);
    actor_lookup_.erase(actor_id);
}

OctreeNode* Octree::insert_light(LightID light_id) {
    auto light = stage_->light(light_id);
    OctreeNode* node = get_or_create_node(light.get());
    node->data->light_ids_.insert(light_id);
    light_lookup_.insert(std::make_pair(light_id, node));
    return node;
}

void Octree::remove_light(LightID light_id) {
    auto node = locate_light(light_id);

    node->data->light_ids_.erase(light_id);
    light_lookup_.erase(light_id);
}

NodeLevel Octree::calculate_level(float diameter) {
    NodeLevel level = 0;

    // If there is no root, then we're outside the bounds
    if(!has_root()) {
        throw OutsideBoundsError();
    }

    float octree_diameter = get_root()->diameter();

    // If we're outside the root octree radius, then we're outside the bounds
    if(diameter > octree_diameter) {
        throw OutsideBoundsError();
    }

    // Calculate the level by dividing the root radius until
    // we hit the point that the object is smaller
    while(diameter < octree_diameter) {
        octree_diameter /= 2.0f;
        ++level;
    }

    return level;
}

float Octree::node_diameter(const NodeType *node) const {
    return (root_width_ / (node->level() + 1));
}

void Octree::prune_empty_nodes() {
    /* FIXME: Prune empty leaf nodes */
}

OctreeNode* Octree::get_or_create_node(Boundable* boundable) {
    /*
     * This is where the magic happens!
     *
     * First, we check if we have a root node, if we don't we create one the same size as the AABB.
     *
     * Otherwise, if we have a root, but the If the AABB doesn't fit inside, we grow the tree upwards until it does.
     *
     * Otherwise, we work out what the maximum level is depending on the size of the AABB
     * this is the final level where the AABB will fit.
     *
     * Then we work backwards from that level to find a node that contains the centre point
     * when we find one, we check if it requires splitting. If so we divide that into 8
     * children.
     *
     *
     */

    auto& aabb = boundable->aabb();

    if(levels_.empty()) {
        // No root at all, let's just create one
        VectorHash hash = generate_vector_hash(aabb.centre());

        auto new_node = std::make_shared<NodeType>(this, 0, aabb.centre());
        LevelNodes nodes;
        nodes.insert(std::make_pair(hash, new_node));

        levels_.push_back(nodes);
        root_width_ = aabb.max_dimension();

        return new_node.get();
    }

    NodeLevel level = calculate_level(aabb.max_dimension());
    VectorHash hash = calculate_node_hash(level, aabb.centre());

    return nullptr;
}

}
}
