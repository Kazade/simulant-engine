
#include "../types.h"

template<typename Triangle>
Octree<Triangle>::Octree(vector_type aabb_min, vector_type aabb_max, uint8_t max_triangles_per_node, void* user_data):
    root_(new node(this, aabb_min, aabb_max)),
    max_triangles_per_node_(max_triangles_per_node),
    user_data_(user_data) {

}

template<typename Triangle>
typename Octree<Triangle>::node* Octree<Triangle>::insert_triangle(const Triangle& triangle) {
    const std::string OVERFLOW_MESSAGE = "Ran out of room in tree, consider increasing max_triangles_per_node";

    node* target = root_;

    while(target->has_children()) {
        node* child = target->find_child_node_for(triangle);
        if(!child) {
            // Triangle physically won't fit, so just return this node
            break;
        } else {
            target = child;
        }
    }

    if(target->count + 1 == max_triangles_per_node_) {
        if(target->has_children()) {
            // We couldn't fit the child any lower, but we hit the max_triangles_per_node, game over.
            throw std::overflow_error(OVERFLOW_MESSAGE);
        } else {
            target->split(); // Create child nodes
            node* child = target->find_child_node_for(triangle); // Find the new child
            if(!child) {
                // Child won't physically fit :(
                throw std::overflow_error(OVERFLOW_MESSAGE);
            } else {
                target = child;
            }
        }
    }

    group_id id = triangle.get_group(user_data_);
    if(!target->groups_.count(id)) {
        target->groups_.insert(std::make_pair(id, std::make_shared<triangle_list>()));
    }

    target->groups_[id]->push_back(triangle);
    ++target->count;

    return target;
}

template<typename Triangle>
typename Octree<Triangle>::triangle_list_array Octree<Triangle>::gather_triangles(
        const Octree<Triangle>::node_list& nodes, group_id group) {

    triangle_list_array ret;
    for(auto node: nodes) {
        if(group == -1) {
            for(auto p: node->groups_) { ret.push_back(p.second.get()); }
        } else {
            if(node->groups_.count(group)) {
                ret.push_back(node->groups_[group].get());
            }
        }
    }
    return ret;
}

template<typename Triangle>
typename Octree<Triangle>::node* Octree<Triangle>::node_containing_point(const Octree<Triangle>::vector_type& p) {
    node* current = root_;
    while(current) {
        if(!current->has_children()) {
            return current;
        }

        current = current->find_child_containing_point(p, ALL_CHILDREN);
    }

    return current;
}

template<typename Triangle>
typename Octree<Triangle>::node_list Octree<Triangle>::find_nodes_intersecting_ray(const Octree<Triangle>::vector_type& start, const Octree<Triangle>::vector_type& dir) {
    kmRay3 ray;
    kmRay3Fill(&ray, start.x, start.y, start.z, dir.x, dir.y, dir.z);

    node_list nodes = root_->find_nodes_that_satisfy_predicate([ray](node* this_node) -> bool {
        kglt::AABB aabb;
        kmVec3Assign(&aabb.min, &this_node->min);
        kmVec3Assign(&aabb.max, &this_node->max);
        return kmRay3IntersectAABB3(&ray, &aabb, nullptr, nullptr);
    });

    return nodes;
}

