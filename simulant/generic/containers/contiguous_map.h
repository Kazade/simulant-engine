#pragma once

/*
 * ContiguousMap<T> is a container with the following properties:
 *
 *  - Sorted on insertion
 *  - Clears without dealloc
 *  - Nodes stored in contiguous memory
 *
 * Internally it's a simple binary search tree that's stored
 * in a vector. Ideally it would be a red-black tree to
 * balance it. FUTURE IMPROVEMENT IF ANYONE WANTS TO PICK IT UP!
 */


#include <vector>
#include <cstdint>
#include <cassert>
#include <stdexcept>

namespace smlt {

namespace _contiguous_map {

template<typename K, typename V>
struct NodeMeta {
    NodeMeta(const K& key, const V& value):
        key(key), value(value) {}

    K key;
    V value;

    int32_t left_index_ = -1;
    int32_t right_index_ = -1;
};

}


template<typename K, typename V, typename Compare=std::less<K>>
class ContiguousMap {
public:
    typedef K key_type;
    typedef V value_type;

    ContiguousMap() = default;

    ContiguousMap(std::size_t reserve_count):
        root_index_(-1) {
        nodes_.reserve(reserve_count);
    }

    ContiguousMap(const ContiguousMap&) = delete;  // Avoid copies for now, slow!
    ContiguousMap& operator=(const ContiguousMap&) = delete;

    bool insert(const K& key, V&& element) {
        K k = key; // Copy K to leverage the move of _insert
        return _insert(std::move(k), std::move(element));
    }

    bool insert(const K& key, const V& element) {
        K k = key;
        V v = element;
        return _insert(std::move(k), std::move(v));
    }

    void clear() {
        nodes_.clear();
    }

    void shrink_to_fit() {
        nodes_.shrink_to_fit();
    }

    std::size_t size() const {
        return nodes_.size();
    }

    bool empty() const {
        return nodes_.empty();
    }

    const V& at(const K& key) const {
        const node_type* node = _find(key);
        if(!node) {
            throw std::out_of_range("Key not found");
        }
        return node->value;
    }

private:
    typedef _contiguous_map::NodeMeta<K, V> node_type;

    const node_type* _find(const K& key) const {
        if(root_index_ == -1) return nullptr;

        const node_type* current = &nodes_[root_index_];

        while(current) {
            if(current->key == key) {
                return current;
            } else if(less_(key, current->key)) {
                if(current->left_index_ != -1) {
                    current = &nodes_[current->left_index_];
                } else {
                    return nullptr;
                }
            } else {
                if(current->right_index_ != -1) {
                    current = &nodes_[current->right_index_];
                } else {
                    return nullptr;
                }
            }
        }

        return current;
    }

    bool _insert(K&& key, V&& value) {
        if(root_index_ == -1) {
            root_index_ = new_node(std::move(key), std::move(value));
            return true;
        } else {
            return _insert_recurse(
                root_index_, std::move(key), std::move(value), false
            );
        }
    }

    int32_t new_node(K&& key, V&& value) {
        nodes_.push_back(node_type(key, value));
        return nodes_.size() - 1;
    }

    bool _insert_recurse(int32_t root_index, K&& key, V&& value, bool overwrite) {
        assert(root_index > -1);

        node_type* root = &nodes_[root_index];
        if(less_(key, root->key)) {
            if(root->left_index_ == -1) {
                auto new_idx = new_node(std::move(key), std::move(value));
                /* The insert could have invalidated the root pointer */
                root = &nodes_[root_index];
                root->left_index_ = new_idx;
                return true;
            } else {
                return _insert_recurse(
                    root->left_index_, std::move(key), std::move(value), overwrite
                );
            }
        } else {
            if(root->right_index_ == -1) {
                if(root->key == key) {
                    if(overwrite) {
                        root->value = std::move(value);
                        return true;
                    } else {
                        return false;
                    }
                } else {
                    auto new_idx = new_node(std::move(key), std::move(value));
                    /* The insert could have invalidated the root pointer */
                    root = &nodes_[root_index];
                    root->right_index_ = new_idx;
                    return true;
                }
            } else {
                return _insert_recurse(
                    root->right_index_, std::move(key), std::move(value), overwrite
                );
            }
        }
    }

    std::vector<node_type> nodes_;

    int32_t root_index_ = -1;
    Compare less_;
};


}
