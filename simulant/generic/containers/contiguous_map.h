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

    bool is_black = false;

    /* If this is not -1, then this is the start
     * of a linked list of equal values */
    int32_t equal_index_ = -1;

    int32_t parent_index_ = -1;
    int32_t left_index_ = -1;
    int32_t right_index_ = -1;
};

}


template<typename K, typename V, typename Compare=std::less<K>>
class ContiguousMultiMap {
public:
    typedef K key_type;
    typedef V value_type;
    typedef _contiguous_map::NodeMeta<K, V> node_type;

    class iterator_base {
    protected:
        /* This is the equivalent of begin */
        iterator_base(ContiguousMultiMap* map):
            map_(map) {

            current_node_ = map->root_index_;

            if(current_node_ > -1 && _node(current_node_)->left_index_ > -1) {
                current_node_ = _node(current_node_)->left_index_;
            }
        }

        /* Passing -1 means end(), anything else points to the specified node */
        iterator_base(ContiguousMultiMap* map, int32_t index):
            map_(map) {

            current_node_ = index;

            if(index > -1) {
                auto current = _node(current_node_);
                if(current->left_index_ > -1) {
                    previous_node_index_ = current->left_index_;
                }
            }
        }

        iterator_base(const iterator_base& other) = default;

        inline typename ContiguousMultiMap::node_type* _node(int32_t index) const {
            assert(index > -1);
            return &map_->nodes_[index];
        }

        void increment() {
            if(current_node_ < 0) return; // Do nothing

            auto current = _node(current_node_);

            /* We have a pointer to an equal value node, but we haven't started iterating */
            if(current->equal_index_ > -1 && list_head_index_ == -1) {
                list_head_index_ = current_node_;
                current_node_ = current->equal_index_;
                return;
            } else if(list_head_index_ > -1) {
                /* We're iterating equal value nodes */
                current_node_ = current->equal_index_;
                if(current_node_ == -1) {
                    /* We've finished iterating the list, now back to recursing */
                    current_node_ = list_head_index_; // Restore back to the head node
                    list_head_index_ = -1; // We're no longer iterating the list
                    current = _node(current_node_); // Set current so we can continue normally
                } else {
                    /* We've updated the current node */
                    return;
                }
            }

            auto previous = current_node_;

            if(current->right_index_ == -1 || current->right_index_ == previous_node_index_) {
                current_node_ = current->parent_index_;
            } else {
                current_node_ = current->right_index_;
                if(current_node_ > -1) {
                    /* Go down the left branch to start iterating this one */
                    current = _node(current_node_);
                    while(current->left_index_ > -1) {
                        current_node_ = current->left_index_;
                        current = _node(current_node_);
                    }
                }
            }

            previous_node_index_ = previous;
        }

        bool is_equal(const iterator_base& other) const {
            return (
                map_ == other.map_ &&
                current_node_ == other.current_node_
            );
        }

        int32_t current_node_ = -1;

    private:
        ContiguousMultiMap* map_ = nullptr;

        int32_t list_head_index_ = -1;
        int32_t current_node_index_ = -1;
        int32_t previous_node_index_ = -1;
    };

    class iterator : private iterator_base {
    private:
        iterator(ContiguousMultiMap* map):
            iterator_base(map) {}

        iterator(ContiguousMultiMap* map, int32_t index):
            iterator_base(map, index) {}

    public:
        friend class ContiguousMultiMap;

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = uint32_t;
        using pointer = std::pair<const K, V>*;
        using reference = std::pair<const K, V>&;

        iterator(const iterator& other) = default;
        iterator& operator=(const iterator&) = default;
        iterator& operator=(iterator&&) = default;

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

        pointer operator->() const {
            return &this->_node(this->current_node_)->pair;
        }
    };

    class const_iterator : private iterator_base {
    private:
        friend class ContiguousMultiMap;

        const_iterator(ContiguousMultiMap* map):
            iterator_base(map) {}

        const_iterator(ContiguousMultiMap* map, int32_t index):
            iterator_base(map, index) {}

    public:

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const K, V>;
        using difference_type = uint32_t;
        using pointer = const std::pair<const K, V>*;
        using reference = const std::pair<const K, V>&;

        const_iterator(const const_iterator& other) = default;
        const_iterator& operator=(const const_iterator&) = default;
        const_iterator& operator=(const_iterator&&) = default;

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

        pointer operator->() const {
            return &this->_node(this->current_node_)->pair;
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
        root_index_ = -1;
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
        return iterator(this, index);
    }

    const_iterator find(const K &key) const {
        int32_t index = _find(key);
        if(index == -1) {
            return end();
        }

        return const_iterator(
            const_cast<ContiguousMultiMap*>(this),
            index
        );
    }

    const_iterator upper_bound(const K& key) const {
        auto it = find(key);

        while(it != end() && it->first == key) {
            ++it;
        }

        return it;
    }

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return end_;
    }

    const_iterator begin() const {
        return const_iterator(
            const_cast<ContiguousMultiMap*>(this)
        );
    }

    const_iterator end() const {
        return cend_;
    }

    /* This is a helper function for debugging the contiguousmap.
     * Given a path of L or R characters, returns the key at that path
     * or throws an error if the path is invalid */

    const K& path_key(const std::string& path) const {
        const node_type* node = &nodes_[root_index_];

        for(auto c: path) {
            if(c == 'L') {
                if(node->left_index_ == -1) {
                    throw std::logic_error("Hit end of branch");
                }
                node = &nodes_[node->left_index_];
            } else if(c == 'R') {
                if(node->right_index_ == -1) {
                    throw std::logic_error("Hit end of branch");
                }
                node = &nodes_[node->right_index_];
            } else {
                throw std::logic_error("Invalid path");
            }
        }

        /* Return the key */
        return node->pair.first;
    }
private:
    friend class iterator_base;

    iterator end_ = iterator(this, -1);
    const_iterator cend_ = const_iterator(
        const_cast<ContiguousMultiMap*>(this), -1
    );

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

    void _rotate_left(node_type* node, int32_t node_index, node_type* parent=nullptr) {
        int32_t nnew_index = node->right_index_;
        assert(nnew_index != -1);

        /* Fetch the parent if it wasn't passed */
        int32_t parent_index = node->parent_index_;
        parent = (parent) ? parent : (parent_index == -1) ? nullptr : &nodes_[parent_index];

        node_type* nnew = &nodes_[nnew_index];

        node->right_index_ = nnew->left_index_;
        nnew->left_index_ = node_index;
        node->parent_index_ = nnew_index;

        if(node->right_index_ != -1) {
            nodes_[node->right_index_].parent_index_ = node_index;
        }

        if(parent) {
            if(node_index == parent->left_index_) {
                parent->left_index_ = nnew_index;
            } else if(node_index == parent->right_index_) {
                parent->right_index_ = nnew_index;
            }
        }

        nnew->parent_index_ = parent_index;
    }

    void _rotate_right(node_type* node, int32_t node_index, node_type* parent=nullptr) {
        int32_t nnew_index = node->left_index_;
        assert(nnew_index != -1);

        /* Fetch the parent if it wasn't passed */
        int32_t parent_index = node->parent_index_;
        parent = (parent) ? parent : (parent_index == -1) ? nullptr : &nodes_[parent_index];

        node_type* nnew = &nodes_[nnew_index];

        node->left_index_ = nnew->right_index_;
        nnew->right_index_ = node_index;
        node->parent_index_ = nnew_index;

        if(node->left_index_ != -1) {
            nodes_[node->left_index_].parent_index_ = node_index;
        }

        if(parent) {
            if(node_index == parent->left_index_) {
                parent->left_index_ = nnew_index;
            } else if(node_index == parent->right_index_) {
                parent->right_index_ = nnew_index;
            }
        }

        nnew->parent_index_ = parent_index;
    }

    void _insert_repair_tree(node_type* node, int32_t node_index) {

        /* Case 1: Parent is the root, so just mark the parent as black */
        if(node->parent_index_ == -1) {
            node->is_black = true;
            return;
        }

        assert(node->parent_index_ != -1);
        node_type* parent = &nodes_[node->parent_index_];

        /* Case 2: Parent is black, we're all good */
        if(parent->is_black) {
            return;
        }

        node_type* grandparent = (parent->parent_index_ == -1) ?
            nullptr : &nodes_[parent->parent_index_];

        int32_t parent_sibling_index = (!grandparent) ? -1 :
            (grandparent->left_index_ == node->parent_index_) ?
            grandparent->right_index_ : grandparent->left_index_;

        node_type* parent_sibling = (parent_sibling_index == -1) ? nullptr : &nodes_[parent_sibling_index];

        /* Case 3: parent has a sibling, and it's red */
        if(parent_sibling && !parent_sibling->is_black) {
            /* Recolor and recurse upwards */
            parent->is_black = true;
            parent_sibling->is_black = true;
            grandparent->is_black = false;

            /* Recurse up! */
            _insert_repair_tree(grandparent, parent->parent_index_);
        } else {
            /* Case 4: Parent is red, but the parent sibling doesn't exist, or is black */
            if(node_index == parent->right_index_ && node->parent_index_ == grandparent->left_index_) {
                _rotate_left(parent, node->parent_index_, grandparent);

                node_index = node->left_index_;
                node = &nodes_[node->left_index_];

                assert(node->parent_index_ != -1);
                parent = &nodes_[node->parent_index_];

                assert(parent->parent_index_ != -1);
                grandparent = &nodes_[parent->parent_index_];

            } else if(node_index == parent->left_index_ && node->parent_index_ == grandparent->right_index_) {
                _rotate_right(parent, node->parent_index_, grandparent);

                node_index = node->right_index_;
                node = &nodes_[node->right_index_];

                assert(node->parent_index_ != -1);
                parent = &nodes_[node->parent_index_];

                assert(parent->parent_index_ != -1);
                grandparent = &nodes_[parent->parent_index_];
            }

            if(node_index == parent->left_index_) {
                _rotate_right(grandparent, parent->parent_index_);
            } else {
                _rotate_left(grandparent, parent->parent_index_);
            }

            assert(parent);
            assert(grandparent);

            parent->is_black = true;
            grandparent->is_black = false;
        }
    }

    bool _insert(K&& key, V&& value) {
        if(root_index_ == -1) {
            /* Root case, just set to black */
            root_index_ = new_node(-1, std::move(key), std::move(value));
            nodes_.back().is_black = true;
            return true;
        } else {
            /* Returns new index, and whether or not the
             * parent is a black node */
            auto p = _insert_recurse(
                root_index_, std::move(key), std::move(value)
            );

            if(p.first > -1 && !p.second) {
                /* Parent was red! rebalance! */
                node_type* inserted = &nodes_[p.first];
                _insert_repair_tree(inserted, p.first);

                /* Reset root index */
                node_type* parent = inserted;
                while(parent->parent_index_ != -1) {
                    node_type* next_parent = &nodes_[parent->parent_index_];

                    if(next_parent->parent_index_ == -1) {
                        root_index_ = parent->parent_index_;
                        break;
                    }

                    parent = next_parent;
                }

                return true;
            }

            /* p.first is the new index, it will be -1 on insert failure */
            return p.first > -1;
        }
    }

    inline int32_t new_node(int32_t parent_index, K&& key, V&& value) {
        auto ret = (int32_t) nodes_.size();
        nodes_.push_back(node_type(key, value));
        nodes_.back().parent_index_ = parent_index;
        return ret;
    }

    /* Returns inserted, parent_is_black */
    std::pair<int32_t, bool> _insert_recurse(int32_t root_index, K&& key, V&& value) {
        assert(root_index > -1);

        node_type* root = &nodes_[root_index];

        // FIXME: Should be equivalence? That will incur a performance
        // hit though :thinking_face:
        if(key == root->pair.first) {

            /* We're inserting a duplicate, so we use the equal_index_ */

            auto new_idx = new_node(-1, std::move(key), std::move(value));
            root = &nodes_[root_index]; // new_node could realloc

            auto dupe_node = &nodes_[new_idx];

            dupe_node->equal_index_ = root->equal_index_;
            root->equal_index_ = new_idx;

            /* We return the new node index, and we say the parent
             * is black (because this node was added to a linked list
             * not the tree itself and we don't want any recursive fixing
             * of the red-black tree) */
            return std::make_pair(new_idx, true);

        } else if(less_(key, root->pair.first)) {
            if(root->left_index_ == -1) {
                auto new_idx = new_node(root_index, std::move(key), std::move(value));
                /* The insert could have invalidated the root pointer */
                root = &nodes_[root_index];
                root->left_index_ = new_idx;
                return std::make_pair(new_idx, root->is_black);
            } else {
                return _insert_recurse(
                    root->left_index_, std::move(key), std::move(value)
                );
            }
        } else {
            if(root->right_index_ == -1) {
                auto new_idx = new_node(root_index, std::move(key), std::move(value));
                /* The insert could have invalidated the root pointer */
                root = &nodes_[root_index];
                root->right_index_ = new_idx;
                return std::make_pair(new_idx, root->is_black);
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
