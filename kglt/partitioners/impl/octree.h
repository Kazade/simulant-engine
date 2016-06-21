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
    std::unordered_map<ActorID, AABB> actor_ids_;
    std::unordered_map<LightID, AABB> light_ids_;
    std::unordered_map<ParticleSystemID, AABB> particle_system_ids_;

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

class MissingParentError : public std::logic_error {
public:
    MissingParentError():
        std::logic_error("Tried to create node when there is a missing parent node") {}
};

typedef uint32_t NodeLevel;

class Octree;
class OctreeLevel;

class OctreeNode {
public:
    typedef std::set<OctreeNode*> NodeList;

    OctreeNode(Octree* octree, OctreeLevel* level, uint32_t diameter, const Vec3 &centre);

    bool is_empty() const {
        return data->is_empty() && !has_children();
    }

    bool is_root() const {
        return !parent();
    }

    bool has_children() const;
    const uint32_t diameter() const { return diameter_; }

    NodeList children() const;
    OctreeNode* parent() const { return parent_; }
    NodeList siblings() const {
        if(parent_) {
            auto ret = parent()->children();
            ret.erase((OctreeNode*)this);
            return ret;
        } else {
            return NodeList();
        }
    }

    Property<OctreeNode, NodeData> data = { this, &OctreeNode::data_ };
    Property<OctreeNode, Octree> octree = { this, &OctreeNode::octree_ };

    NodeLevel level() const;
    Vec3 centre() const { return centre_; }

    const bool contains(const Vec3& p) const;    

    std::vector<Vec3> child_centres() const;

    const AABB aabb() const {
        return AABB(centre_, diameter_);
    }
private:
    Octree* octree_ = nullptr;
    OctreeLevel* level_ = nullptr;

    float diameter_;
    Vec3 centre_;

    std::shared_ptr<NodeData> data_;

    OctreeNode* parent_ = nullptr;
    std::set<OctreeNode*> children_;

    friend class Octree;
};


bool default_split_predicate(OctreeNode* node);
bool default_merge_predicate(const OctreeNode::NodeList& nodes);

typedef std::size_t VectorHash;
typedef std::unordered_map<VectorHash, std::shared_ptr<OctreeNode>> NodeMap;
typedef int32_t NodeDiameter;

struct OctreeLevel {
    OctreeLevel(uint32_t level=0): level_number(level) {}

    uint32_t level_number = 0;
    NodeMap nodes;
};


class Octree {
public:
    typedef OctreeNode NodeType;
    typedef typename OctreeNode::NodeList NodeList;

    Octree(Stage* stage,
        std::function<bool (NodeType*)> should_split_predicate = &default_split_predicate,
        std::function<bool (const NodeList&)> should_merge_predicate = &default_merge_predicate
    );

    NodeType* insert_actor(ActorID actor_id);
    NodeType* insert_light(LightID light_id);
    NodeType* insert_particle_system(ParticleSystemID particle_system_id);

    NodeType* locate_actor(ActorID actor_id) {
        if(!actor_lookup_.count(actor_id)) return nullptr;

        return actor_lookup_.at(actor_id);
    }

    NodeType* locate_light(LightID light_id) {
        if(!light_lookup_.count(light_id)) return nullptr;
        return light_lookup_.at(light_id);
    }

    NodeType* locate_particle_system(ParticleSystemID particle_system_id) {
        if(!particle_system_lookup_.count(particle_system_id)) return nullptr;

        return particle_system_lookup_.at(particle_system_id);
    }

    void remove_actor(ActorID actor_id);
    void remove_light(LightID light_id);
    void remove_particle_system(ParticleSystemID particle_system_id);

    void prune_empty_nodes();

    const Vec3 centre() const;
    const NodeDiameter diameter() const {
        return (get_root()) ? get_root()->diameter() : 0.0f;
    }

    bool is_empty() const { return levels_.empty(); }
    NodeLevel node_level(NodeType* node) const;
    NodeList nodes_at_level(NodeLevel level) const;

    bool has_root() const { return !levels_.empty(); }

    NodeType* get_root() const {
        if(levels_.empty() || levels_.front()->nodes.empty()) return nullptr;
        return levels_.front()->nodes.begin()->second.get();
    }

    const uint32_t node_count() const { return node_count_; }

    kglt::MeshID debug_mesh_id() { return debug_mesh_; }
private:
    friend class ::NewOctreeTest;
    friend class OctreeNode;

    typedef std::deque<std::shared_ptr<OctreeLevel>> LevelArray;

    VectorHash generate_vector_hash(const Vec3& vec);
    Stage* stage_;
    LevelArray levels_;

    uint32_t calculate_level(float diameter);
    VectorHash calculate_node_hash(uint32_t level, const Vec3& centre) { return generate_vector_hash(centre); }
    NodeDiameter node_diameter(uint32_t level) const; // This is the "tight" diameter, not the loose bound

    std::pair<uint32_t, VectorHash> find_best_existing_node(const AABB& aabb);
    Vec3 find_node_centre_for_point(NodeLevel level, const Vec3& p);

    NodeType* get_or_create_node(Boundable* boundable);
    std::pair<NodeType*, bool> get_or_create_node(NodeLevel level, const Vec3& centre, NodeDiameter diameter);

    std::unordered_map<ActorID, NodeType*> actor_lookup_;
    std::unordered_map<LightID, NodeType*> light_lookup_;
    std::unordered_map<ParticleSystemID, NodeType*> particle_system_lookup_;

    std::function<bool (NodeType*)> should_split_predicate_;
    std::function<bool (const NodeList&)> should_merge_predicate_;

    bool split_if_necessary(NodeType* node);
    bool merge_if_possible(const NodeList& node);
    void reinsert_data(std::shared_ptr<NodeData> data);
    void grow_to_contain(const AABB& aabb);

    bool inside_octree(const AABB& aabb) const;

    uint32_t node_count_ = 0;

    NodeType* create_node(int32_t level, Vec3 centre, NodeDiameter diameter);
    void remove_node(NodeType* node);

    kglt::MeshID debug_mesh_;
};


void traverse(OctreeNode* start, std::function<bool (OctreeNode*)> callback);

}
}
