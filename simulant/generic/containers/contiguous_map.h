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
class ContiguousMultiMap {
public:
    typedef K key_type;
    typedef V value_type;

    class iterator_base {
    protected:
        iterator_base(ContiguousMultiMap<K, V>& map, bool is_end=false):
            map_(map),
            is_end_(is_end) {

            if(!is_end_) {
                prev_nodes_.push(-1);
                current_node_ = map.root_index_;

                if(current_node_ > -1 && _node(current_node_)->left_index_ > -1) {
                    increment();
                }
            } else {
                current_node_ = -1;
            }
        }

        iterator_base(ContiguousMultiMap<K, V>& map, int32_t index):
            map_(map) {

            assert(index > -1);
            prev_nodes_.push(-1);
            current_node_ = index;
        }

        inline typename ContiguousMultiMap<K, V>::node_type* _node(int32_t index) const {
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

        void increment() {
            if(current_node_ < 0) return; // Do nothing

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
        }

        bool is_equal(const iterator_base& other) const {
            return (
                &map_ == &other.map_ &&
                current_node_ == other.current_node_
            );
        }

        int32_t current_node_ = -1;

    private:
        ContiguousMultiMap<K, V>& map_;
        bool is_end_ = false;

        std::stack<int32_t> prev_nodes_;
    };

    class iterator : private iterator_base {
    private:
        iterator(ContiguousMultiMap<K, V>& map, bool is_end=false):
            iterator_base(map, is_end) {}

        iterator(ContiguousMultiMap<K, V>& map, int32_t index):
            iterator_base(map, index) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = uint32_t;
        using pointer = std::pair<const K, V>*;
        using reference = std::pair<const K, V>&;

        iterator& operator++() {
            this->increment();
            return *this;
        }

        bool operator==(const iterator& other) const {
            return this->is_equal(other);
        }

        bool operator!=(const iterator& other) const {
            return !this->is_equal(other);
        }

        reference operator*() const {
            return this->_node(this->current_node_)->pair;
        }
    };

    class const_iterator : private iterator_base {
    private:
        const_iterator(ContiguousMultiMap<K, V>& map, bool is_end=false):
            iterator_base(map, is_end) {}

        const_iterator(ContiguousMultiMap<K, V>& map, int32_t index):
            iterator_base(map, index) {}

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = uint32_t;
        using pointer = const std::pair<const K, V>*;
        using reference = const std::pair<const K, V>&;

        const_iterator& operator++() {
            this->increment();
            return *this;
        }

        bool operator==(const const_iterator& other) const {
            return this->is_equal(other);
        }

        bool operator!=(const const_iterator& other) const {
            return !this->is_equal(other);
        }

        reference operator*() const {
            return this->_node(this->current_node_)->pair;
        }
    };

    ContiguousMultiMap() = default;

    ContiguousMultiMap(std::size_t reserve_count):
        root_index_(-1) {
        nodes_.reserve(reserve_count);
    }

    ContiguousMultiMap(const ContiguousMultiMap&) = delete;  // Avoid copies for now, slow!
    ContiguousMultiMap& operator=(const ContiguousMultiMap&) = delete;

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

    iterator find(const K& key) {
        int32_t index = _find(key);
        if(index == -1) {
            return end();
        }
        return iterator(*this, index);
    }

    const_iterator find(const K &key) const {
        int32_t index = _find(key);
        if(index == -1) {
            return end();
        }

        return const_iterator(
            const_cast<ContiguousMultiMap&>(*this),
            index
        );
    }

    iterator begin() {
        return iterator(*this);
    }

    iterator end() {
        return iterator(*this, true);
    }

    const_iterator begin() const {
        return const_iterator(*this);
    }

    const_iterator end() const {
        return const_iterator(
            const_cast<ContiguousMultiMap&>(*this),
            true
        );
    }
private:
    typedef _contiguous_map::NodeMeta<K, V> node_type;

    int32_t _find(const K& key) const {
        if(root_index_ == -1) return -1;

        int32_t current_index = root_index_;
        const node_type* current = &nodes_[current_index];

        while(current) {
            if(current->pair.first == key) {
                return current_index;
            } else if(less_(key, current->pair.first)) {
                if(current->left_index_ != -1) {
                    current_index = current->left_index_;
                    current = &nodes_[current_index];
                } else {
                    return -1;
                }
            } else {
                if(current->right_index_ != -1) {
                    current_index = current->right_index_;
                    current = &nodes_[current_index];
                } else {
                    return -1;
                }
            }
        }

        return current_index;
    }

    bool _insert(K&& key, V&& value) {
        if(root_index_ == -1) {
            root_index_ = new_node(std::move(key), std::move(value));
            return true;
        } else {
            return _insert_recurse(
                root_index_, std::move(key), std::move(value)
            );
        }
    }

    int32_t new_node(K&& key, V&& value) {
        nodes_.push_back(node_type(key, value));
        return nodes_.size() - 1;
    }

    bool _insert_recurse(int32_t root_index, K&& key, V&& value) {
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
                    root->left_index_, std::move(key), std::move(value)
                );
            }
        } else {
            if(root->right_index_ == -1) {
                auto new_idx = new_node(std::move(key), std::move(value));
                /* The insert could have invalidated the root pointer */
                root = &nodes_[root_index];
                root->right_index_ = new_idx;
                return true;
            } else {
                return _insert_recurse(
                    root->right_index_, std::move(key), std::move(value)
                );
            }
        }
    }

    std::vector<node_type> nodes_;

    int32_t root_index_ = -1;
    Compare less_;
};


template<typename K, typename V, typename Compare=std::less<K>>
class ContiguousMap {
public:
    ContiguousMap() = default;
    ContiguousMap(std::size_t reserve):
        map_(reserve) {}

    bool empty() const {
        return map_.empty();
    }

    std::size_t size() const {
        return map_.size();
    }

    bool insert(const K& key, V&& element) {
        if(map_.find(key) == map_.end()) {
            map_.insert(key, std::move(element));
            return true;
        } else {
            return false;
        }
    }

    const V& at(const K& key) const {
        auto it = map_.find(key);

        if(it == map_.end()) {
            throw std::out_of_range("Key not found");
        }

        return (*it).second;
    }

    bool insert(const K& key, const V& element) {
        if(map_.find(key) == map_.end()) {
            map_.insert(key, element);
            return true;
        } else {
            return false;
        }
    }

    void shrink_to_fit() {
        map_.shrink_to_fit();
    }

    void clear() {
        map_.clear();
    }
private:
    ContiguousMultiMap<K, V> map_;
};


}
