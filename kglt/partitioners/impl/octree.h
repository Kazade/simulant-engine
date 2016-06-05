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

namespace kglt {
namespace impl {

struct NodeData : public StaticChunkHolder {
    std::list<ActorID> actor_ids_;
    std::list<LightID> light_ids_;
    std::list<ParticleSystemID> particle_system_ids_;

    bool is_empty() const {
        return actor_ids_.empty() && light_ids_.empty() && particle_system_ids_.empty();
    }
};

class Octree;

class OctreeNode {
public:
    typedef std::list<OctreeNode*> NodeList;

    OctreeNode(Octree* octree);

    bool is_empty() const {
        return data->is_empty() && !has_children();
    }

    bool is_root() const {
        return !parent();
    }

    bool has_children() const;
    const float diameter() const;

    NodeList immediate_children() const;
    NodeList children() const;
    NodeList siblings() const;

    OctreeNode* parent() const;

    Property<OctreeNode, NodeData> data = { this, &OctreeNode::data_ };
    Property<OctreeNode, Octree> octree = { this, &OctreeNode::octree_ };

private:
    Octree* octree_ = nullptr;
    std::shared_ptr<NodeData> data_;

    OctreeNode* parent_ = nullptr;
    OctreeNode* children_[8] = { nullptr };
};


class Octree {
public:
    typedef uint32_t Level;
    typedef OctreeNode NodeType;
    typedef typename OctreeNode::NodeList NodeList;

    Octree(StagePtr stage);

    NodeType* insert_actor(ActorID actor_id);
    NodeType* insert_light(LightID light_id);

    NodeType* locate_actor(ActorID actor_id);
    NodeType* locate_light(LightID light_id);

    void remove_actor(ActorID actor_id);
    void remove_light(ActorID actor_id);

    void prune_empty_nodes();

    const Vec3 centre() const;
    const float diameter() const;

    bool is_empty() const;
    Level node_level(NodeType* node) const;
    NodeList nodes_at_level(Level level) const;

private:    
    typedef std::size_t VectorHash;
    typedef std::map<VectorHash, std::shared_ptr<NodeType>> LevelNodes;
    typedef std::vector<LevelNodes> LevelArray;
    typedef typename LevelNodes::size_type NodeIndex;

    VectorHash generate_vector_hash(const Vec3& vec);
    StagePtr stage_;
    LevelArray levels_;

    Level calculate_level(float radius);
    VectorHash calculate_node_hash(Level level, const Vec3& centre);

    NodeType* get_or_create_node(Boundable* boundable);
};

}
}
