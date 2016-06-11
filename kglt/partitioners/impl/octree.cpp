#include "octree.h"
#include "../../types.h"
#include "../../stage.h"
#include "../../actor.h"
#include "../../light.h"

namespace kglt {
namespace impl {

bool default_split_predicate(OctreeNode* node) {
    return true;
}

bool default_merge_predicate(const std::vector<OctreeNode*>& nodes) {
    return true;
}


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

    data_.reset(new NodeData());
}

const bool OctreeNode::contains(const Vec3& p) const {
    float hw = this->diameter() / 2.0;
    float minx = centre_.x - hw;
    float maxx = centre_.x + hw;

    if(p.x < minx || p.x > maxx) return false;

    float miny = centre_.y - hw;
    float maxy = centre_.y + hw;

    if(p.y < miny || p.y > maxy) return false;

    float minz = centre_.z - hw;
    float maxz = centre_.z + hw;

    if(p.z < minz || p.z > maxz) return false;

    return true;
}

std::vector<Vec3> OctreeNode::child_centres() const {
    float qw = this->diameter() / 4.0;

    std::vector<Vec3> centres;

    for(auto x: {-1, 1}) {
        for(auto y: {-1, 1}) {
            for(auto z: {-1, 1}) {
                centres.push_back(this->centre() + Vec3(x * qw, y * qw, z * qw));
            }
        }
    }

    return centres;
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
    return octree_->node_diameter(this->level());
}


Octree::Octree(StagePtr stage,
    std::function<bool (NodeType *)> should_split_predicate,
    std::function<bool (const std::vector<NodeType*>&)> should_merge_predicate):

    stage_(stage),
    should_split_predicate_(should_split_predicate),
    should_merge_predicate_(should_merge_predicate) {

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

    if(node) {
        // Insert into both the node, and the lookup table
        node->data->actor_ids_.insert(std::make_pair(actor_id, actor->aabb()));
        actor_lookup_.insert(std::make_pair(actor_id, node));
    }

    if(split_if_necessary(node)) {
        return locate_actor(actor_id);
    } else {
        return node;
    }
}

void Octree::remove_actor(ActorID actor_id) {
    auto node = locate_actor(actor_id);

    if(node) {
        // Remove the actor from both the node, and the lookup table
        node->data->actor_ids_.erase(actor_id);
        actor_lookup_.erase(actor_id);
    }
}

OctreeNode* Octree::insert_light(LightID light_id) {
    auto light = stage_->light(light_id);
    OctreeNode* node = get_or_create_node(light.get());

    if(node) {
        // Insert the light id into both the node and the lookup table
        node->data->light_ids_.insert(std::make_pair(light_id, light->aabb()));
        light_lookup_.insert(std::make_pair(light_id, node));
    }

    if(split_if_necessary(node)) {
        return locate_light(light_id);
    } else {
        return node;
    }
}

void Octree::remove_light(LightID light_id) {
    auto node = locate_light(light_id);

    if(node) {
        // Remove the light from the node and lookup table
        node->data->light_ids_.erase(light_id);
        light_lookup_.erase(light_id);
    }

    merge_if_possible(node->siblings());
}

std::pair<NodeLevel, Octree::VectorHash> Octree::find_best_existing_node(const AABB& aabb) {
    float diameter = aabb.max_dimension();
    NodeLevel max_level = calculate_level(diameter);

    if(is_empty()) {
        throw OutsideBoundsError();
    }

    Octree::VectorHash hash;

    // Go to the max level, and gradually step down until we find an existing node
    auto final_level = max_level;
    while(final_level >= 0) {
        if(levels_.size() > final_level) {
            // We got to an existing level, let's use that!

            // OK so we have a level, now, let's work out which node this aabb belongs in
            auto node_centre = find_node_centre_for_point(final_level, aabb.centre());
            hash = generate_vector_hash(node_centre);
            // Does it exist already?
            if(levels_[final_level].count(hash)) {
                break;
            } else {
                // We continue on to the next highest level
            }
        }

        final_level--;
    }

    return std::make_pair(final_level, hash);
}

Vec3 Octree::find_node_centre_for_point(NodeLevel level, const Vec3& p) {
    /*
     * Given a level and a position, calculate the centre point for the
     * containing node at that level
     */

    if(is_empty()) {
        // If we have no root node, we can't calculate this - we need the root
        // node centre position to work this out
        throw OutsideBoundsError();
    }

    if(!get_root()->contains(p)) {
        // If we're outside the root then we need to deal with that elsewhere
        throw OutsideBoundsError();
    }

    float step = node_diameter(level);

    auto snap = [step, level](const float& in) -> float {
        return step * std::round(in / step) + ((level) ? step / 2.0 : 0.0) * ((in >= 0) ? 1 : -1);
    };

    return Vec3(
        snap(p.x),
        snap(p.y),
        snap(p.z)
    );
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

float Octree::node_diameter(NodeLevel level) const {
    return (root_width_ / std::pow(2, level));
}

void Octree::prune_empty_nodes() {
    /* FIXME: Prune empty leaf nodes */
}

bool Octree::split_if_necessary(NodeType* node) {
    if(!should_split_predicate_(node)) {
        return false;
    }

    //FIXME: Split things

    return true;
}

bool Octree::merge_if_possible(const NodeList &nodes) {
    if(!should_merge_predicate_(nodes)) {
        return false;
    }

    //FIXME: merge things

    return true;
}

OctreeNode* Octree::get_or_create_node(Boundable* boundable) {
    auto& aabb = boundable->aabb();

    if(aabb.has_zero_area()) {
        throw InvalidBoundableInsertion("Object has no spacial area. Cannot insert into Octree.");
    }

    if(levels_.empty()) {
        // No root at all, let's just create one
        VectorHash hash = generate_vector_hash(aabb.centre());

        auto new_node = std::make_shared<NodeType>(this, 0, aabb.centre());
        LevelNodes nodes;
        nodes.insert(std::make_pair(hash, new_node));

        levels_.push_back(nodes);
        root_width_ = aabb.max_dimension();
        ++node_count_;

        return new_node.get();
    }

    // Find the best node to insert this boundable
    auto level_and_hash = find_best_existing_node(aabb);

    return levels_[level_and_hash.first].at(level_and_hash.second).get();
}

}
}
