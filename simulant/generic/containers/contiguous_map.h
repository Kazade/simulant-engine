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
#include <stack>
#include <utility>
#include <cstdint>
#include <cassert>
#include <stdexcept>

namespace smlt {

namespace _contiguous_map {

template<typename K, typename V>
struct NodeMeta {
    NodeMeta(const K& key, const V& value):
        pair(std::make_pair(key, value)) {}

    std::pair<const K, V> pair;

    int32_t left_index_ = -1;
    int32_t right_index_ = -1;
};

}


template<typename K, typename V, typename Compare=std::less<K>>
class ContiguousMap {
public:
    typedef K key_type;
    typedef V value_type;

    class iterator {
    private:
        iterator(ContiguousMap<K, V>& map, bool is_end=false):
            map_(map),
            is_end_(is_end) {

            if(!is_end_) {
                prev_nodes_.push(-1);
                current_node_ = map.root_index_;

                if(current_node_ > -1 && _node(current_node_)->left_index_ > -1) {
                    ++(*this);
                }
            } else {
                current_node_ = -1;
            }
        }

        inline typename ContiguousMap<K, V>::node_type* _node(int32_t index) const {
            assert(index > -1);
            return &map_.nodes_[index];
        }

        int32_t _next(int32_t index) {
            auto start = _node(index);
            if(start->left_index_ > -1) {
                prev_nodes_.push(index);
                return _next(start->left_index_);
            } else {
                return index;
            }
        }

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = uint32_t;
        using pointer = std::pair<const K, V>*;
        using reference = std::pair<const K, V>&;

        iterator& operator++() {
            if(current_node_ < 0) return *this; // Do nothing

            auto current = _node(current_node_);
            if(current->left_index_ > -1 && prev_nodes_.top() != current_node_) {
                prev_nodes_.push(current_node_);
                current_node_ = _next(current->left_index_);
            } else {
                if(current_node_ == prev_nodes_.top()) {
                    prev_nodes_.pop();
                }

                if(current->right_index_ > -1) {
                    current_node_ = _next(current->right_index_);
                } else {
                    current_node_ = prev_nodes_.top();
                }
            }

            return *this;
        }

        bool operator==(const iterator& other) const {
            return  (
                &map_ == &other.map_ &&
                current_node_ == other.current_node_
            );
        }

        bool operator!=(const iterator& other) const {
            return (
                &map_ != &other.map_ ||
                current_node_ != other.current_node_
            );
        }

        reference operator*() const {
            return _node(current_node_)->pair;
        }

    private:
        ContiguousMap<K, V>& map_;
        bool is_end_ = false;

        int32_t current_node_;
        std::stack<int32_t> prev_nodes_;
    };

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
        return node->pair.second;
    }

    iterator begin() {
        return iterator(*this);
    }

    iterator end() {
        return iterator(*this, true);
    }

private:
    typedef _contiguous_map::NodeMeta<K, V> node_type;

    const node_type* _find(const K& key) const {
        if(root_index_ == -1) return nullptr;

        const node_type* current = &nodes_[root_index_];

        while(current) {
            if(current->pair.first == key) {
                return current;
            } else if(less_(key, current->pair.first)) {
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
        if(less_(key, root->pair.first)) {
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
                if(root->pair.first == key) {
                    if(overwrite) {
                        root->pair.second = std::move(value);
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
