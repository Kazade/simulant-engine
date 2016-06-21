#include "octree.h"
#include "grid.h"
#include "../../types.h"
#include "../../stage.h"
#include "../../actor.h"
#include "../../light.h"
#include "../../particles.h"


namespace kglt {
namespace impl {

bool default_split_predicate(OctreeNode* node) {
    return true;
}

bool default_merge_predicate(const OctreeNode::NodeList &nodes) {
    return true;
}

float next_pow2(float value) {
    int x = std::ceil(value);
    if(x == 0) {
        return 1;
    }

    return pow(2, ceil(log(x)/log(2)));
}

template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

OctreeNode::OctreeNode(Octree* octree, OctreeLevel *level, uint32_t diameter, const Vec3& centre):
    octree_(octree),
    level_(level),
    diameter_(diameter),
    centre_(centre) {

    data_.reset(new NodeData());
}

const bool OctreeNode::contains(const Vec3& p) const {
    NodeDiameter hw = this->diameter() / 2;
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
    int32_t qw = (int32_t) this->diameter() / 4;

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
    return !children_.empty();
}

OctreeNode::NodeList OctreeNode::children() const {
    return children_;
}

NodeLevel OctreeNode::level() const {
    return level_->level_number;
}

Octree::Octree(Stage *stage,
    std::function<bool (NodeType *)> should_split_predicate,
    std::function<bool (const NodeList&)> should_merge_predicate):

    stage_(stage),
    should_split_predicate_(should_split_predicate),
    should_merge_predicate_(should_merge_predicate) {

}

VectorHash Octree::generate_vector_hash(const Vec3& vec) {
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

    return VectorHash(seed);
}

OctreeNode* Octree::insert_actor(ActorID actor_id) {
    auto actor = stage_->actor(actor_id);
    OctreeNode* node = get_or_create_node(actor.get());

    if(node) {
        // Insert into both the node, and the lookup table
        node->data->actor_ids_.insert(std::make_pair(actor_id, actor->aabb()));
        actor_lookup_[actor_id] = node;
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

        auto siblings = node->siblings();
        siblings.insert(node);
        merge_if_possible(siblings);
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

        auto siblings = node->siblings();
        siblings.insert(node);
        merge_if_possible(siblings);

        if(node->is_empty()) {
            prune_empty_nodes();
        }
    }
}

OctreeNode* Octree::insert_particle_system(ParticleSystemID particle_system_id) {
    auto ps = stage_->particle_system(particle_system_id);
    OctreeNode* node = get_or_create_node(ps.get());

    if(node) {
        // Insert the light id into both the node and the lookup table
        node->data->particle_system_ids_.insert(std::make_pair(particle_system_id, ps->aabb()));
        particle_system_lookup_.insert(std::make_pair(particle_system_id, node));
    }

    if(split_if_necessary(node)) {
        return locate_particle_system(particle_system_id);
    } else {
        return node;
    }
}

void Octree::remove_particle_system(ParticleSystemID ps_id) {
    auto node = locate_particle_system(ps_id);

    if(node) {
        // Remove the actor from both the node, and the lookup table
        node->data->particle_system_ids_.erase(ps_id);
        particle_system_lookup_.erase(ps_id);

        auto siblings = node->siblings();
        siblings.insert(node);
        merge_if_possible(siblings);

        if(node->is_empty()) {
            prune_empty_nodes();
        }
    }
}

bool Octree::inside_octree(const AABB& aabb) const {
    if(is_empty()) {
        return false;
    }

    OctreeNode* root = get_root();
    auto rd = root->diameter();
    auto maxd = aabb.max_dimension();
    return root->contains(aabb.centre()) && (rd * 2) >= maxd;
}

std::pair<NodeLevel, VectorHash> Octree::find_best_existing_node(const AABB& aabb) {
    float diameter = aabb.max_dimension();
    NodeLevel max_level = calculate_level(diameter);

    if(!inside_octree(aabb)) {
        throw OutsideBoundsError();
    }

    VectorHash hash;

    auto final_level = max_level;
    if(final_level == 0) {
        return std::make_pair(0, generate_vector_hash(get_root()->centre()));
    }

    bool node_found = false;
    // Go to the max level, and gradually step down until we find an existing node
    while(final_level > 0) {
        if(levels_.size() > final_level) {
            // We got to an existing level, let's use that!

            // OK so we have a level, now, let's work out which node this aabb belongs in
            auto node_centre = find_node_centre_for_point(final_level, aabb.centre());
            hash = generate_vector_hash(node_centre);
            // Does it exist already?
            if(levels_[final_level]->nodes.count(hash)) {
                node_found = true;
                break;
            } else {
                // We continue on to the next highest level
            }
        }

        final_level--;
    }

    // If we went through the loop and no node was found, then we need to return
    // the hash of the root node
    if(!node_found) {
        hash = generate_vector_hash(get_root()->centre());
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

    /* If we're at the root, we just return the centre, everything is based on this
     * so calculating anything else might be wrong */
    if(level == 0) {
        return get_root()->centre();
    }

    float step = node_diameter(level);

    Grid grid(step, get_root()->diameter() / 2, step / 2.0, step / 2.0, step / 2.0);
    auto ret = grid.snap(p);

    return ret;
}

NodeLevel Octree::calculate_level(float diameter) {
    NodeLevel level = 0;

    // If there is no root, then we're outside the bounds
    if(!has_root()) {
        throw OutsideBoundsError();
    }

    uint32_t octree_diameter = get_root()->diameter();

    // If we're outside the root octree radius, then we're outside the bounds
    if(diameter > (octree_diameter * 2)) {
        throw OutsideBoundsError();
    }

    // Calculate the level by dividing the root radius until
    // we hit the point that the object is smaller
    while(diameter < (octree_diameter * 2)) {
        octree_diameter /= 2;
        ++level;
    }

    return level;
}

NodeDiameter Octree::node_diameter(NodeLevel level) const {
    NodeDiameter root_width = get_root()->diameter();
    if(level == 0) {
        return root_width;
    }

    return (root_width / std::pow(2, level));
}

void Octree::prune_empty_nodes() {
    auto level = levels_.size();
    while(level--) {
        bool deleted = false;
        auto level_nodes = levels_[level]->nodes; // Copy! This is so we can use remove_node() in the loop
        for(auto node_pair: level_nodes) {
            if(node_pair.second->is_empty()) {
                remove_node(node_pair.second.get());
                deleted = true;
            }
        }
        if(!deleted) {
            // We got to a level where no nodes needed deleting
            break;
        }
    }
}

void Octree::reinsert_data(std::shared_ptr<NodeData> data) {
    for(auto& actor_pair: data->actor_ids_) {
        insert_actor(actor_pair.first);
    }

    for(auto& light_pair: data->light_ids_) {
        insert_light(light_pair.first);
    }

    for(auto& particle_pair: data->particle_system_ids_) {
        insert_particle_system(particle_pair.first);
    }
}

bool Octree::split_if_necessary(NodeType* node) {
    bool should_split = should_split_predicate_(node);
    if(!should_split) {
        return false;
    }

    if(node->diameter() == 1) {
        // We can't split any further than 1 - don't even try!
        return false;
    }

    bool created = false;
    NodeLevel node_level = node->level();

    // Create children
    std::vector<OctreeNode*> nodes_created;
    for(auto v: node->child_centres()) {
        auto pair = get_or_create_node(node_level + 1, v, node->diameter() / 2.0f);

        if(pair.second) {
            created = true;
            nodes_created.push_back(pair.first);
            node->children_.insert(pair.first);
        }
    }

    // If no children were created, then it's likely the stuff
    // in this node is already as low down as it can be, the predicate
    // might keep returning true but there's not much we can do about it
    if(!created) {
        return false;
    }

    // Now, relocate everything!
    auto data = node->data_; // Stash original data
    node->data_.reset(new NodeData()); // Wipe the data from the original node

    // Reinsert the data into the tree, now that we have a lower level of nodes
    reinsert_data(data);

    // Now, remove any nodes which are now unnecessary
    for(auto node: nodes_created) {
        if(node->is_empty()) {
            remove_node(node);
        }
    }

    L_DEBUG(_u("Node count {0}. Level count {1}").format(node_count_, levels_.size()));

    return true;
}

bool Octree::merge_if_possible(const NodeList &nodes) {
    if(nodes.empty()) {
        return false;
    }

    if(!should_merge_predicate_(nodes)) {
        return false;
    }

    auto first = (*nodes.begin());
    auto parent = first->parent();
    if(!parent) {
        // Removing the root node, but only if it's empty
        if(first->is_empty()) {
            remove_node(first);
        }
        return true;
    }

    std::vector<std::shared_ptr<NodeData>> datas;

    for(auto& node: nodes) {
        if(!node->is_empty()) {
            datas.push_back(node->data_);
            node->data_.reset();
        }

        // If this is the case, then the node has children and we shouldn't even
        // be trying to merge anything
        assert(node->is_empty());
        remove_node(node);
    }

    for(auto& data: datas) {
        parent->data->actor_ids_.insert(data->actor_ids_.begin(), data->actor_ids_.end());
        parent->data->light_ids_.insert(data->light_ids_.begin(), data->light_ids_.end());
        parent->data->particle_system_ids_.insert(data->particle_system_ids_.begin(), data->particle_system_ids_.end());
    }

    return true;
}

std::pair<OctreeNode*, bool> Octree::get_or_create_node(NodeLevel level, const Vec3& centre, NodeDiameter diameter) {
    auto hash = generate_vector_hash(centre);

    assert(level >= 0);

    if(level < levels_.size()) {
        if(levels_[level]->nodes.count(hash)) {
            return std::make_pair(levels_[level]->nodes[hash].get(), false);
        }
    }

    return std::make_pair(create_node(level, centre, diameter), true);
}

void Octree::grow_to_contain(const AABB& aabb) {
    OctreeNode* root = get_root();

    while(!inside_octree(aabb)) {
        auto c = aabb.centre();
        auto rc = (root) ? root->centre() : Vec3();

        NodeDiameter new_diameter = (root) ? root->diameter() * 2 : next_pow2(aabb.max_dimension() / 2.0f);
        NodeDiameter qd = new_diameter / 4;

        Vec3 new_centre_offset(
            (c.x < rc.x) ? -qd : qd,
            (c.y < rc.y) ? -qd : qd,
            (c.z < rc.z) ? -qd : qd
        );

        // If we're creating the first node, we don't want to offset
        if(!root) {
            new_centre_offset = Vec3();
        }

        create_node(-1, rc + new_centre_offset, new_diameter);

        root = get_root();

        L_DEBUG(_u("Octree diameter {0}").format(root->diameter()));
    }
}

OctreeNode* Octree::get_or_create_node(Boundable* boundable) {
    auto& aabb = boundable->aabb();

    if(!get_root() || !inside_octree(aabb)) {
        // OK, so the boundable is outside the current octree, so we need to grow
        grow_to_contain(aabb);
    }

    // Find the best node to insert this boundable
    auto level_and_hash = find_best_existing_node(aabb);

    return levels_[level_and_hash.first]->nodes.at(level_and_hash.second).get();
}

OctreeNode* Octree::create_node(int32_t level_number, Vec3 centre, NodeDiameter diameter) {
    if(!debug_mesh_) {
        debug_mesh_ = stage_->new_mesh(false);
    }

    auto hash = generate_vector_hash(centre);

    OctreeLevel* level = nullptr;
    if(level_number < 0) {
        // Increment the level number on existing levels
        for(auto& level: levels_) {
            level->level_number++;
        }

        // Insert out new level at the front (default number is zero so that's fine)
        levels_.push_front(std::make_shared<OctreeLevel>());
        level = levels_.front().get();
    } else if(uint32_t(level_number) == levels_.size()) {
        levels_.push_back(std::make_shared<OctreeLevel>(level_number));
        level = levels_.back().get();
    } else {
        level = levels_.at(level_number).get();
    }

    auto* nodes = &level->nodes;

    auto new_node = std::make_shared<OctreeNode>(this, level, diameter, centre);
    nodes->insert(std::make_pair(hash, new_node));
    ++node_count_;

    stage_->mesh(debug_mesh_)->new_submesh_as_box(
        stage_->clone_default_material(),
        diameter, diameter, diameter, centre
    );

    // Update the parent if this isn't a root node
    if(level->level_number > 0) {
        auto hash = generate_vector_hash(find_node_centre_for_point(level->level_number - 1, centre));
        new_node->parent_ = levels_[level->level_number - 1]->nodes.at(hash).get();
        new_node->parent_->children_.insert(new_node.get());
    } else if(levels_.size() > 1) {
        // If we just added a new root node, then update the previous root
        // to have a parent now
        for(auto& node: levels_[1]->nodes) {
            node.second->parent_ = levels_.front()->nodes.begin()->second.get();
        }
    }

    return new_node.get();
}

void Octree::remove_node(NodeType* node) {
    auto level = node->level();

    if(node->parent_) {
        node->parent_->children_.erase(node);
    } else {
        // Removing the root node! assert that we're not doing anything bad
        assert(levels_[0]->nodes.size() == 1);
    }

    for(auto& actor_pair: node->data->actor_ids_) {
        actor_lookup_.erase(actor_pair.first);
    }

    for(auto& light_pair: node->data->light_ids_) {
        light_lookup_.erase(light_pair.first);
    }

    for(auto& particle_system_pair: node->data->particle_system_ids_) {
        particle_system_lookup_.erase(particle_system_pair.first);
    }

    levels_[level]->nodes.erase(generate_vector_hash(node->centre()));

    // Remove the level if it's empty and the last one
    if(level == levels_.size() - 1 && levels_[level]->nodes.empty()) {
        levels_.pop_back();
    } else if(level == 0) {
        assert(levels_[0]->nodes.empty());
        levels_.pop_front();
        if(!levels_.empty()) {
            // We should never have more than one root node
            assert(levels_[0]->nodes.size() == 1);
        }
    }

    --node_count_;
}

void traverse(OctreeNode* start, std::function<bool (OctreeNode*)> callback) {
    /*
     * Traverses the tree from the starting node in the traditional root-to-leaf way.
     * The callback must return true if the traversal should continue to the nodes children
     * returning false will terminate that particular branch's traversal.
     */

    std::function<void (OctreeNode*)> do_traversal = [&](OctreeNode* node) {
        if(callback(node)) {
            for(auto& child: node->children()) {
                do_traversal(child);
            }
        }
    };

    do_traversal(start);
}

}
}
