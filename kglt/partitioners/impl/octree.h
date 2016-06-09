#pragma once

/*
 * A Super Octree!
 *
 * This Octree is:
 *
 * - Fast. Inserting and locating happens in constant(ish) time
 * - Loose. Nodes are twice the size as a traditional octree
 * - Dynamic. The Octree grows and shrinks to contain the objects
 * - Generic. It's up to you to define the data on each node
 * - Cache-friendly. Nodes for a particular level are stored in consecutive memory
 */

#include <list>
#include "../../types.h"
#include "../../generic/property.h"
#include "../static_chunk.h"

class NewOctreeTest;

namespace kglt {
namespace impl {

struct NodeData : public StaticChunkHolder {
    std::set<ActorID> actor_ids_;
    std::set<LightID> light_ids_;
    std::set<ParticleSystemID> particle_system_ids_;

    bool is_empty() const {
        return actor_ids_.empty() && light_ids_.empty() && particle_system_ids_.empty();
    }
};

class OutsideBoundsError : public std::logic_error {
public:
    OutsideBoundsError():
        std::logic_error("Object was outside the bounds of the root node") {}
};

class InvalidBoundableInsertion : public std::logic_error {
public:
    InvalidBoundableInsertion(const std::string& what):
        std::logic_error(what) {}
};

typedef uint32_t NodeLevel;

class Octree;

class OctreeNode {
public:
    typedef std::list<OctreeNode*> NodeList;

    OctreeNode(Octree* octree, NodeLevel level, const Vec3 &centre);

    bool is_empty() const {
        return data->is_empty() && !has_children();
    }

    bool is_root() const {
        return !parent();
    }

    bool has_children() const;
    const float diameter() const;

    NodeList children() const;
    OctreeNode* parent() const { return parent_; }

    Property<OctreeNode, NodeData> data = { this, &OctreeNode::data_ };
    Property<OctreeNode, Octree> octree = { this, &OctreeNode::octree_ };

    NodeLevel level() const { return level_; }
    Vec3 centre() const { return centre_; }

    const bool contains(const Vec3& p) const;
private:
    Octree* octree_ = nullptr;
    NodeLevel level_ = 0;
    Vec3 centre_;

    std::unique_ptr<NodeData> data_;

    OctreeNode* parent_ = nullptr;
    OctreeNode* children_[8] = { nullptr };
};


class Octree {
public:
    typedef OctreeNode NodeType;
    typedef typename OctreeNode::NodeList NodeList;

    Octree(StagePtr stage);

    NodeType* insert_actor(ActorID actor_id);
    NodeType* insert_light(LightID light_id);

    NodeType* locate_actor(ActorID actor_id) { return actor_lookup_.at(actor_id); }
    NodeType* locate_light(LightID light_id) { return light_lookup_.at(light_id); }

    void remove_actor(ActorID actor_id);
    void remove_light(LightID light_id);

    void prune_empty_nodes();

    const Vec3 centre() const;
    const float diameter() const { return root_width_; }

    bool is_empty() const { return levels_.empty(); }
    NodeLevel node_level(NodeType* node) const;
    NodeList nodes_at_level(NodeLevel level) const;

    bool has_root() const { return !levels_.empty(); }
    NodeType* get_root() const { return levels_.front().begin()->second.get(); }

private:
    friend class ::NewOctreeTest;
    friend class OctreeNode;

    typedef std::size_t VectorHash;
    typedef std::unordered_map<VectorHash, std::shared_ptr<NodeType>> LevelNodes;
    typedef std::vector<LevelNodes> LevelArray;
    typedef typename LevelNodes::size_type NodeIndex;

    float root_width_ = 0.0f;

    VectorHash generate_vector_hash(const Vec3& vec);
    StagePtr stage_;
    LevelArray levels_;

    NodeLevel calculate_level(float diameter);
    VectorHash calculate_node_hash(NodeLevel level, const Vec3& centre) { return generate_vector_hash(centre); }
    float node_diameter(NodeLevel level) const; // This is the "tight" diameter, not the loose bound

    std::pair<NodeLevel, VectorHash> find_best_existing_node(const AABB& aabb);
    Vec3 find_node_centre_for_point(NodeLevel level, const Vec3& p);

    NodeType* get_or_create_node(Boundable* boundable);

    std::unordered_map<ActorID, NodeType*> actor_lookup_;
    std::unordered_map<LightID, NodeType*> light_lookup_;
};

}
}
