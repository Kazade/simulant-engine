#pragma once

#include <set>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include "../types.h"

/*
 * A static octree for handling only triangles. This is a template so that
 * you can pass your own Triangle class, it just must have the following:
 *
* Triangle must provide the following interface:
 *
 * Vec3 get_vertex(i, user_data);
 * group_id get_group() const; // Should return 0 if not grouping
 */

typedef int32_t group_id;

template<typename Triangle>
class NullGrouper {
public:
    inline group_id get_group(const Triangle& tri) { return 0; }
};

enum OctreeChild {
    OCTREE_CHILD_UPPER_BACK_LEFT,
    OCTREE_CHILD_UPPER_FRONT_LEFT,
    OCTREE_CHILD_UPPER_FRONT_RIGHT,
    OCTREE_CHILD_UPPER_BACK_RIGHT,
    OCTREE_CHILD_LOWER_BACK_LEFT,
    OCTREE_CHILD_LOWER_FRONT_LEFT,
    OCTREE_CHILD_LOWER_FRONT_RIGHT,
    OCTREE_CHILD_LOWER_BACK_RIGHT
};

const std::set<OctreeChild> ALL_CHILDREN = {
    OCTREE_CHILD_UPPER_BACK_LEFT,
    OCTREE_CHILD_UPPER_FRONT_LEFT,
    OCTREE_CHILD_UPPER_FRONT_RIGHT,
    OCTREE_CHILD_UPPER_BACK_RIGHT,
    OCTREE_CHILD_LOWER_BACK_LEFT,
    OCTREE_CHILD_LOWER_FRONT_LEFT,
    OCTREE_CHILD_LOWER_FRONT_RIGHT,
    OCTREE_CHILD_LOWER_BACK_RIGHT
};


template<typename Triangle>
class Octree {
public:
    typedef kglt::Vec3 vector_type;
    typedef std::vector<Triangle> triangle_list;
    typedef std::unordered_map<group_id, std::shared_ptr<triangle_list>> triangle_group_dict;
    typedef std::vector<triangle_list*> triangle_list_array;

    struct Node;
    typedef std::vector<Node*> node_list;

    struct Node {
        Node(Octree* octree, const vector_type& min, const vector_type& max):
            octree(octree), min(min), max(max) {

            assert(max.x - min.x == max.y - min.y);
            assert(max.x - min.x == max.z - min.z);
        }

        ~Node() {
            for(auto child: children) {
                delete child;
            }
        }

        Octree* octree = nullptr;
        vector_type min;
        vector_type max;
        Node* children[8] = { nullptr };
        triangle_group_dict groups_;
        uint8_t count = 0;


        inline bool has_children() const { return bool(children[0]); }

        inline OctreeChild find_child_containing_point(const vector_type& v) {
            OctreeChild ret;
            auto c = centre();

            if(v.x >= c.x) {
                if(v.y >= c.y) {
                    if(v.z >= c.z) { ret = OCTREE_CHILD_UPPER_FRONT_LEFT;
                    } else { ret = OCTREE_CHILD_UPPER_BACK_LEFT; }
                } else {
                    if(v.z >= c.z) { ret = OCTREE_CHILD_LOWER_FRONT_LEFT;
                    } else { ret = OCTREE_CHILD_LOWER_BACK_LEFT; }
                }
            } else {
                if(v.y >= c.y) {
                    if(v.z >= c.z) { ret = OCTREE_CHILD_UPPER_FRONT_RIGHT;
                    } else { ret = OCTREE_CHILD_UPPER_BACK_RIGHT; }
                } else {
                    if(v.z >= c.z) { ret = OCTREE_CHILD_LOWER_FRONT_RIGHT;
                    } else { ret = OCTREE_CHILD_LOWER_BACK_RIGHT; }
                }
            }

            return ret;
        }

        inline Node* find_child_node_containing_point(const vector_type& v) {
            return children[find_child_containing_point(v)];
        }

        inline Octree<Triangle>::node_list find_nodes_that_satisfy_predicate(std::function<bool (Node*)> predicate) {
            node_list ret;

            std::function<void (node*)> check = [&](node* current) {
                if(predicate(current)) {
                    ret.push_back(current);
                    if(current->has_children()) {
                        for(auto child: children) {
                            check(child);
                        }
                    }
                }
            };

            check(octree->root_);

            return ret;
        }

        inline Node* find_child_node_for(const Triangle& triangle) {
            assert(has_children());

            bool first = true;
            OctreeChild child;
            for(uint8_t i = 0; i < 3; ++i) {
                auto v = triangle.get_vertex(i, octree->user_data_);

                if(!first) {
                    OctreeChild this_v = find_child_containing_point(v);
                    if(this_v != child) {
                        return nullptr;
                    }
                } else {
                    child = find_child_containing_point(v);
                }
                first = false;
            }

            return children[child];
        }

        std::pair<vector_type, vector_type> find_bounds_for_child(OctreeChild i) {
            float child_size = size() / 2.0f;
            float child_offset = child_size / 2.0f;

            vector_type child_centre = centre();
            (i > 3) ? child_centre.y -= child_offset : child_centre.y += child_offset;

            if(i == 0 || i == 1 || i == 4 || i == 5) child_centre.x -= child_offset;
            else child_centre.x += child_offset;

            if(i == 0 || i == 3 || i == 4 || i == 7) child_centre.z -= child_offset;
            else child_centre.z += child_offset;

            vector_type offset_vec(child_offset, child_offset, child_offset);
            return std::make_pair(
                child_centre - offset_vec,
                child_centre + offset_vec
            );
        }

        inline void split() {
            if(has_children()) return;
            for(uint8_t i = 0; i < 8; ++i) {
                auto bounds = find_bounds_for_child((OctreeChild)i);
                children[i] = new Node(this->octree, bounds.first, bounds.second);
            }

            auto groups = groups_;
            groups_.clear();
            count = 0;

            for(auto& p: groups) {
                for(auto& tri: *(p.second)) {
                    octree->insert_triangle(tri);
                }
            }
        }

        inline float size() const { return (max.x - min.x); } //Dimensions all the same
        inline vector_type centre() const { float hw = size() / 2; return min + vector_type(hw, hw, hw); }
    };

    typedef Node node;

    Octree(vector_type aabb_min, vector_type aabb_max, uint8_t max_triangles_per_node, void* user_data=nullptr);
    ~Octree() {
        delete root_;
    }

    node* insert_triangle(const Triangle& triangle);
    node_list find_nodes_intersecting_ray(const vector_type& start, const vector_type& dir);
    triangle_list_array gather_triangles(const node_list& nodes, group_id group=-1);
    node* node_containing_point(const vector_type& p);

private:
    friend class Node;

    node* root_ = nullptr;
    uint8_t max_triangles_per_node_ = 100;
    void* user_data_ = nullptr;
};

template<typename triangle_list>
std::vector<typename triangle_list::value_type> flatten(const std::vector<triangle_list*> triangle_list_array) {
    std::vector<typename triangle_list::value_type> ret;
    for(auto array: triangle_list_array) {
        ret.insert(ret.end(), array->begin(), array->end());
    }
    return ret;
}

#include "tri_octree.inl"
