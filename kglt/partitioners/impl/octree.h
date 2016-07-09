#pragma once

/*
 * A Super Octree!
 *
 * This Octree is:
 *
 * - Fast. Inserting and locating happens in constant(ish) time
 * - Loose. Nodes are twice the size as a traditional octree
 * - Dynamic. The Octree grows and shrinks to contain the objects
 */

#include <list>
#include <memory>
#include <mutex>

#include "../../types.h"
#include "../../generic/property.h"
#include "../static_chunk.h"

class NewOctreeTest;

namespace kglt {
namespace impl {

struct NodeData : public StaticChunkHolder {
private:
    std::unordered_map<ActorID, AABB> actor_ids_;
    std::unordered_map<LightID, AABB> light_ids_;
    std::unordered_map<ParticleSystemID, AABB> particle_system_ids_;

public:
    NodeData() = default;
    NodeData(const NodeData& rhs):
        actor_ids_(rhs.actor_ids_),
        light_ids_(rhs.light_ids_),
        particle_system_ids_(rhs.particle_system_ids_) {

        // Just to make sure we don't copy the mutex... not sure that they are copyable but whatever.
    }

    NodeData& operator=(const NodeData& rhs) {
        if(this == &rhs) {
            return *this;
        }

        actor_ids_ = rhs.actor_ids_;
        light_ids_ = rhs.light_ids_;
        particle_system_ids_ = rhs.particle_system_ids_;

        return *this;
    }

    bool is_empty() const {
        return actor_ids_.empty() && light_ids_.empty() && particle_system_ids_.empty();
    }

    uint32_t actor_count() const { return actor_ids_.size(); }
    uint32_t light_count() const { return light_ids_.size(); }
    uint32_t particle_system_count() const { return particle_system_ids_.size(); }

    void erase_all() {
        actor_ids_.clear();
        light_ids_.clear();
        particle_system_ids_.clear();
    }

    void insert_or_update(ActorID actor, AABB aabb) {
        actor_ids_[actor] = aabb;
    }

    void erase(ActorID actor_id) {
        actor_ids_.erase(actor_id);
    }

    void insert_or_update(LightID light, AABB aabb) {
        light_ids_[light] = aabb;
    }

    void erase(LightID light_id) {
        light_ids_.erase(light_id);
    }

    void insert_or_update(ParticleSystemID psid, AABB aabb) {
        particle_system_ids_[psid] = aabb;
    }

    void erase(ParticleSystemID ps_id) {
        particle_system_ids_.erase(ps_id);
    }

    void each_actor(std::function<void (ActorID actor_id, AABB aabb)> callback) {
        for(auto& pair: actor_ids_) {
            callback(pair.first, pair.second);
        }
    }

    void each_light(std::function<void (LightID actor_id, AABB aabb)> callback) {
        for(auto& pair: light_ids_) {
            callback(pair.first, pair.second);
        }
    }

    void each_particle_system(std::function<void (ParticleSystemID actor_id, AABB aabb)> callback) {
        for(auto& pair: particle_system_ids_) {
            callback(pair.first, pair.second);
        }
    }

    void merge(NodeData& other) {
        actor_ids_.insert(other.actor_ids_.begin(), other.actor_ids_.end());
        light_ids_.insert(other.light_ids_.begin(), other.light_ids_.end());
        particle_system_ids_.insert(other.particle_system_ids_.begin(), other.particle_system_ids_.end());
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
class OctreeNode;


typedef std::list<
    std::weak_ptr<OctreeNode>
> NodeList;

void node_list_erase(NodeList& node_list, std::weak_ptr<OctreeNode> node);
NodeList::iterator node_list_find(NodeList& node_list, OctreeNode* node);
NodeList::iterator node_list_find(NodeList &node_list, std::weak_ptr<OctreeNode> node);

class OctreeNode : public std::enable_shared_from_this<OctreeNode> {
public:
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
            NodeList ret = parent_->children();
            std::weak_ptr<OctreeNode> ref = std::const_pointer_cast<OctreeNode>(shared_from_this());
            node_list_erase(ret, ref);
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
    void add_child(std::weak_ptr<OctreeNode> child);
    void remove_child(std::weak_ptr<OctreeNode> child);

    Octree* octree_ = nullptr;
    OctreeLevel* level_ = nullptr;

    float diameter_;
    Vec3 centre_;

    std::shared_ptr<NodeData> data_;

    OctreeNode* parent_ = nullptr;
    NodeList children_;

    friend class Octree;
};

bool default_split_predicate(OctreeNode* node);
bool default_merge_predicate(const NodeList& nodes);

typedef std::size_t VectorHash;
typedef std::unordered_map<VectorHash, std::shared_ptr<OctreeNode>> NodeMap;
typedef int32_t NodeDiameter;

struct OctreeLevel {
    OctreeLevel(uint32_t level=0): level_number(level) {}

    uint32_t level_number = 0;
    NodeMap nodes;
};


enum QueuedUpdateType {
    QUEUED_UPDATE_MOVE_ACTOR,
    QUEUED_UPDATE_MOVE_LIGHT,
    QUEUED_UPDATE_MOVE_PARTICLE_SYSTEM
};


struct QueuedUpdate {
    QueuedUpdateType type_;
    ActorID actor_id_;
    LightID light_id_;
    ParticleSystemID particle_system_id_;
};


class Octree {
public:
    typedef OctreeNode NodeType;

    Octree(Stage* stage,
        std::function<bool (NodeType*)> should_split_predicate = &default_split_predicate,
        std::function<bool (const NodeList&)> should_merge_predicate = &default_merge_predicate
    );

    std::weak_ptr<OctreeNode> insert_actor(ActorID actor_id);
    std::weak_ptr<OctreeNode> insert_light(LightID light_id);
    std::weak_ptr<OctreeNode> insert_particle_system(ParticleSystemID particle_system_id);

    std::weak_ptr<OctreeNode> locate_actor(ActorID actor_id) {
        if(!actor_lookup_.count(actor_id)) return std::weak_ptr<OctreeNode>();

        return actor_lookup_.at(actor_id);
    }

    std::weak_ptr<OctreeNode> locate_light(LightID light_id) {
        if(!light_lookup_.count(light_id)) return std::weak_ptr<OctreeNode>();
        return light_lookup_.at(light_id);
    }

    std::weak_ptr<OctreeNode> locate_particle_system(ParticleSystemID particle_system_id) {
        if(!particle_system_lookup_.count(particle_system_id)) return std::weak_ptr<OctreeNode>();

        return particle_system_lookup_.at(particle_system_id);
    }

    void remove_actor(ActorID actor_id);
    void remove_light(LightID light_id);
    void remove_particle_system(ParticleSystemID particle_system_id);

    const Vec3 centre() const;
    const NodeDiameter diameter() const {
        return (get_root()) ? get_root()->diameter() : 0.0f;
    }

    bool is_empty() const { return levels_.empty(); }
    NodeLevel node_level(NodeType* node) const;
    NodeList nodes_at_level(NodeLevel level) const;

    bool has_root() const { return !levels_.empty(); }

    std::shared_ptr<OctreeNode> get_root() const {
        if(levels_.empty() || levels_.front()->nodes.empty()) return std::shared_ptr<OctreeNode>();
        return levels_.front()->nodes.begin()->second;
    }

    const uint32_t node_count() const { return node_count_; }

    kglt::MeshID debug_mesh_id() { return debug_mesh_; }

private:
    friend class ::NewOctreeTest;
    friend class OctreeNode;

    typedef std::deque<std::shared_ptr<OctreeLevel>> LevelArray;

    std::recursive_mutex mutex_;

    VectorHash generate_vector_hash(const Vec3& vec);
    Stage* stage_;
    LevelArray levels_;

    uint32_t calculate_level(float diameter);
    VectorHash calculate_node_hash(uint32_t level, const Vec3& centre) { return generate_vector_hash(centre); }
    NodeDiameter node_diameter(uint32_t level) const; // This is the "tight" diameter, not the loose bound

    std::pair<uint32_t, VectorHash> find_best_existing_node(const AABB& aabb);
    Vec3 find_node_centre_for_point(NodeLevel level, const Vec3& p);

    std::shared_ptr<OctreeNode> get_or_create_node(BoundableEntity* boundable);
    std::pair<std::shared_ptr<OctreeNode>, bool> get_or_create_node(NodeLevel level, const Vec3& centre, NodeDiameter diameter);

    std::unordered_map<ActorID, std::weak_ptr<OctreeNode>> actor_lookup_;
    std::unordered_map<LightID, std::weak_ptr<OctreeNode>> light_lookup_;
    std::unordered_map<ParticleSystemID, std::weak_ptr<OctreeNode>> particle_system_lookup_;

    std::function<bool (NodeType*)> should_split_predicate_;
    std::function<bool (const NodeList&)> should_merge_predicate_;

    bool split_if_necessary(NodeType* node);
    bool merge_if_possible(const NodeList& node);
    void reinsert_data(std::shared_ptr<NodeData> data);
    void grow_to_contain(const AABB& aabb);

    bool inside_octree(const AABB& aabb) const;

    uint32_t node_count_ = 0;

    std::shared_ptr<OctreeNode> create_node(int32_t level, Vec3 centre, NodeDiameter diameter);
    void remove_node(std::weak_ptr<OctreeNode> node);

    kglt::MeshID debug_mesh_;
    kglt::MaterialID debug_material_;

    std::unordered_map<OctreeNode*, kglt::SubMeshID> debug_submeshes_;
    std::unordered_map<ActorID, sig::scoped_connection> actor_watchers_;

    friend void traverse(Octree &tree, std::function<bool (OctreeNode *)> callback);
};


void traverse(Octree &tree, std::function<bool (OctreeNode *)> callback);

}
}
