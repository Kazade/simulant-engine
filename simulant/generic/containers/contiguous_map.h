#pragma once

/*
 * ContiguousMap<T> is a container with the following properties:
 *
 *  - Sorted on insertion
 *  - Clears without dealloc
 *  - Nodes stored in contiguous memory
 *  - Hot/cold data split for cache-efficient lookups
 *
 *  Internally this is a red-black tree. Search-critical data (keys
 *  and child indices) are stored in a separate compact array from
 *  node metadata (values, parent pointers, color bits, equal-value
 *  lists). This means the common find() hot path only touches the
 *  small SearchNode array, keeping more nodes resident per cache
 *  line (critical on SH4 with its 32-byte lines and 16KB operand
 *  cache).
 *
 *  TODO:
 *
 *   - Add the ability to remove a key. This should mark the space in the
 *     vector as "free"
 */

#include <vector>
#include <utility>
#include <cstdint>
#include <cassert>
#include <stdexcept>
#include <string>

#include "../../macros.h"

namespace smlt {

namespace _contiguous_map {

/*
 * Search-hot data: only these fields are touched during key lookup.
 * Keeping this struct small means more nodes fit per cache line,
 * dramatically reducing cache misses during tree traversal.
 */
template<typename K>
struct SearchNode {
    SearchNode() = default;
    SearchNode(const K& k) : key(k) {}

    K key;
    int32_t left_index_ = -1;
    int32_t right_index_ = -1;
};

/*
 * Cold data: accessed during insertion, rebalancing, and iteration.
 * Stored in a separate array so it doesn't pollute the cache during
 * the common find() hot path.
 */
template<typename K, typename V>
struct DataNode {
    DataNode(const K& key, const V& value) :
        pair(std::make_pair(key, value)) {}

    std::pair<const K, V> pair;

    int32_t parent_index_ = -1;
    bool is_black = false;

    /* If this is not -1, then this is the start
     * of a linked list of equal values */
    int32_t equal_index_ = -1;
    int32_t last_equal_index_ = -1;
};

}


template<typename T>
class ThreeWayCompare {
public:
    enum Result : int8_t {
        RESULT_LESS = -1,
        RESULT_EQUAL = 0,
        RESULT_GREATER = 1
    };

    Result operator()(const T& a, const T& b) const {
        return (a < b) ? RESULT_LESS : (b < a) ? RESULT_GREATER : RESULT_EQUAL;
    }
};


template<typename K, typename V, typename Compare=ThreeWayCompare<K>>
class ContiguousMultiMap {
public:
    typedef K key_type;
    typedef V value_type;
    typedef _contiguous_map::SearchNode<K> search_node_type;
    typedef _contiguous_map::DataNode<K, V> data_node_type;

    class iterator_base {
    protected:
        /* This is the equivalent of begin */
        iterator_base(ContiguousMultiMap* map):
            map_(map) {

            current_node_ = map->leftmost_index_;
        }

        /* Passing -1 means end(), anything else points to the specified node */
        iterator_base(ContiguousMultiMap* map, int32_t index):
            map_(map) {

            current_node_ = index;

            if(index > -1) {
                auto* search = _search_node(current_node_);
                if(search->left_index_ > -1) {
                    previous_node_index_ = search->left_index_;
                }
            } else {
                previous_node_index_ = -1;
            }
        }

        iterator_base(const iterator_base& other) = default;

        inline typename ContiguousMultiMap::search_node_type* _search_node(int32_t index) const {
            assert(index > -1);
            return &map_->search_nodes_[index];
        }

        inline typename ContiguousMultiMap::data_node_type* _data_node(int32_t index) const {
            assert(index > -1);
            return &map_->data_nodes_[index];
        }

        void increment() {
            if(current_node_ < 0) return; // Do nothing

            auto* data = _data_node(current_node_);

            /* We have a pointer to an equal value node, but we haven't started iterating */
            if(data->equal_index_ > -1 && list_head_index_ == -1) {
                list_head_index_ = current_node_;
                current_node_ = data->equal_index_;
                return;
            } else if(list_head_index_ > -1) {
                /* We're iterating equal value nodes */
                current_node_ = data->equal_index_;
                if(current_node_ == -1) {
                    /* We've finished iterating the list, now back to recursing */
                    current_node_ = list_head_index_; // Restore back to the head node
                    list_head_index_ = -1; // We're no longer iterating the list
                    data = _data_node(current_node_); // Set data so we can continue normally
                } else {
                    /* We've updated the current node */
                    return;
                }
            }

            auto* search = _search_node(current_node_);
            auto previous = current_node_;

            if(search->right_index_ == -1) {
                /* We've reached the end, now we go up until we come from
                 * the left branch */

                current_node_ = data->parent_index_;
                while(current_node_ > -1) {
                    search = _search_node(current_node_);
                    if(previous == search->left_index_) {
                        /* We came from the left, so break */
                        break;
                    } else {
                        previous = current_node_;
                        current_node_ = _data_node(current_node_)->parent_index_;
                    }
                }

            } else {
                current_node_ = search->right_index_;
                if(current_node_ > -1) {
                    /* Go down the left branch to start iterating this one */
                    search = _search_node(current_node_);
                    while(search->left_index_ > -1) {
                        current_node_ = search->left_index_;
                        search = _search_node(current_node_);
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
            return this->_data_node(this->current_node_)->pair;
        }

        pointer operator->() const {
            return &this->_data_node(this->current_node_)->pair;
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
            return this->_data_node(this->current_node_)->pair;
        }

        pointer operator->() const {
            return &this->_data_node(this->current_node_)->pair;
        }
    };

    ContiguousMultiMap() = default;

    ContiguousMultiMap(std::size_t reserve_count):
        root_index_(-1),
        leftmost_index_(-1) {
        search_nodes_.reserve(reserve_count);
        data_nodes_.reserve(reserve_count);
    }

    ContiguousMultiMap(const ContiguousMultiMap& other) {
        clear();
        for(auto& it: other) {
            insert(it.first, it.second);
        }
    }

    ContiguousMultiMap& operator=(const ContiguousMultiMap& other) {
        if(&other == this) {
            return *this;
        }

        clear();
        for(auto& it: other) {
            insert(it.first, it.second);
        }

        return *this;
    }

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
        search_nodes_.clear();
        data_nodes_.clear();
        root_index_ = -1;
        leftmost_index_ = -1;
    }

    std::size_t count(const K& key) const {
        return find(key) != end() ? 1 : 0;
    }

    void shrink_to_fit() {
        search_nodes_.shrink_to_fit();
        data_nodes_.shrink_to_fit();
    }

    std::size_t size() const {
        return search_nodes_.size();
    }

    void reserve(std::size_t size) {
        search_nodes_.reserve(size);
        data_nodes_.reserve(size);
    }

    bool empty() const {
        return search_nodes_.empty();
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
        int32_t idx = root_index_;

        for(auto c: path) {
            const auto* search = &search_nodes_[idx];
            if(c == 'L') {
                if(search->left_index_ == -1) {
                    throw std::logic_error("Hit end of branch");
                }
                idx = search->left_index_;
            } else if(c == 'R') {
                if(search->right_index_ == -1) {
                    throw std::logic_error("Hit end of branch");
                }
                idx = search->right_index_;
            } else {
                throw std::logic_error("Invalid path");
            }
        }

        /* Return the key */
        return search_nodes_[idx].key;
    }
private:
    friend class iterator_base;

    iterator end_ = iterator(this, -1);
    const_iterator cend_ = const_iterator(
        const_cast<ContiguousMultiMap*>(this), -1
    );

    /*
     * _find only touches search_nodes_ for optimal cache behaviour.
     * On SH4, SearchNode<int> is 12 bytes so ~2.5 nodes fit per
     * 32-byte cache line vs. ~1 node with the old interleaved layout.
     */
    int32_t _find(const K& key) const {
        if(root_index_ == -1) return -1;

        int32_t current_index = root_index_;

        while(current_index != -1) {
            const auto* node = &search_nodes_[current_index];

            auto order = compare_(key, node->key);

            if(order == ThreeWayCompare<K>::RESULT_EQUAL) {
                return current_index;
            } else if(order == ThreeWayCompare<K>::RESULT_LESS) {
                current_index = node->left_index_;
            } else {
                current_index = node->right_index_;
            }

            /* Prefetch the next search node to hide memory latency.
             * On SH4 this emits the pref instruction. We prefetch
             * after choosing the direction so we don't waste cache
             * space loading the unused branch. */
            if(current_index != -1) {
                __builtin_prefetch(&search_nodes_[current_index], 0, 0);
            }
        }

        return -1;
    }

    void _rotate_left(int32_t node_index) {
        auto& node_s = search_nodes_[node_index];
        auto& node_d = data_nodes_[node_index];

        int32_t nnew_index = node_s.right_index_;
        assert(nnew_index != -1);

        auto& nnew_s = search_nodes_[nnew_index];
        auto& nnew_d = data_nodes_[nnew_index];

        int32_t parent_index = node_d.parent_index_;

        node_s.right_index_ = nnew_s.left_index_;
        nnew_s.left_index_ = node_index;
        node_d.parent_index_ = nnew_index;

        if(node_s.right_index_ != -1) {
            data_nodes_[node_s.right_index_].parent_index_ = node_index;
        }

        if(parent_index != -1) {
            auto& parent_s = search_nodes_[parent_index];
            if(node_index == parent_s.left_index_) {
                parent_s.left_index_ = nnew_index;
            } else if(node_index == parent_s.right_index_) {
                parent_s.right_index_ = nnew_index;
            }
        }

        nnew_d.parent_index_ = parent_index;
    }

    void _rotate_right(int32_t node_index) {
        auto& node_s = search_nodes_[node_index];
        auto& node_d = data_nodes_[node_index];

        int32_t nnew_index = node_s.left_index_;
        assert(nnew_index != -1);

        auto& nnew_s = search_nodes_[nnew_index];
        auto& nnew_d = data_nodes_[nnew_index];

        int32_t parent_index = node_d.parent_index_;

        node_s.left_index_ = nnew_s.right_index_;
        nnew_s.right_index_ = node_index;
        node_d.parent_index_ = nnew_index;

        if(node_s.left_index_ != -1) {
            data_nodes_[node_s.left_index_].parent_index_ = node_index;
        }

        if(parent_index != -1) {
            auto& parent_s = search_nodes_[parent_index];
            if(node_index == parent_s.left_index_) {
                parent_s.left_index_ = nnew_index;
            } else if(node_index == parent_s.right_index_) {
                parent_s.right_index_ = nnew_index;
            }
        }

        nnew_d.parent_index_ = parent_index;
    }

    /* Iterative red-black tree repair after insertion */
    void _insert_repair_tree(int32_t node_index) {

        while(true) {
            auto& node_d = data_nodes_[node_index];

            /* Case 1: Parent is the root, so just mark the node as black */
            if(node_d.parent_index_ == -1) {
                node_d.is_black = true;
                return;
            }

            int32_t parent_index = node_d.parent_index_;
            assert(parent_index != -1);
            auto& parent_d = data_nodes_[parent_index];

            /* Case 2: Parent is black, we're all good */
            if(parent_d.is_black) {
                return;
            }

            int32_t grandparent_index = parent_d.parent_index_;
            if(grandparent_index == -1) {
                /* Parent is red and root — shouldn't happen in a valid
                 * tree but handle gracefully */
                parent_d.is_black = true;
                return;
            }

            auto& gp_s = search_nodes_[grandparent_index];
            auto& gp_d = data_nodes_[grandparent_index];

            int32_t uncle_index = (gp_s.left_index_ == parent_index)
                ? gp_s.right_index_ : gp_s.left_index_;

            bool uncle_is_red = (uncle_index != -1) && !data_nodes_[uncle_index].is_black;

            /* Case 3: parent has a sibling (uncle), and it's red */
            if(uncle_is_red) {
                /* Recolor and iterate upwards (was recursive in original) */
                parent_d.is_black = true;
                data_nodes_[uncle_index].is_black = true;
                gp_d.is_black = false;

                node_index = grandparent_index;
                continue;
            }

            /* Case 4: Parent is red, but the uncle doesn't exist, or is black */
            auto& parent_s = search_nodes_[parent_index];

            if(node_index == parent_s.right_index_ && parent_index == gp_s.left_index_) {
                _rotate_left(parent_index);

                node_index = search_nodes_[node_index].left_index_;

                /* Re-derive parent and grandparent after rotation */
                parent_index = data_nodes_[node_index].parent_index_;
                assert(parent_index != -1);
                grandparent_index = data_nodes_[parent_index].parent_index_;
                assert(grandparent_index != -1);

            } else if(node_index == parent_s.left_index_ && parent_index == gp_s.right_index_) {
                _rotate_right(parent_index);

                node_index = search_nodes_[node_index].right_index_;

                /* Re-derive parent and grandparent after rotation */
                parent_index = data_nodes_[node_index].parent_index_;
                assert(parent_index != -1);
                grandparent_index = data_nodes_[parent_index].parent_index_;
                assert(grandparent_index != -1);
            }

            if(node_index == search_nodes_[parent_index].left_index_) {
                _rotate_right(grandparent_index);
            } else {
                _rotate_left(grandparent_index);
            }

            assert(parent_index != -1);
            assert(grandparent_index != -1);

            data_nodes_[parent_index].is_black = true;
            data_nodes_[grandparent_index].is_black = false;
            return;
        }
    }

    bool _insert(K&& key, V&& value) {
        if(root_index_ == -1) {
            /* Root case, just set to black */
            leftmost_index_ = root_index_ = _new_node(-1, key, value);
            data_nodes_.back().is_black = true;
            return true;
        } else {
            /* Returns new index, and whether or not the
             * parent is a black node */
            int32_t new_index;
            bool is_black = _insert_iterative(
                root_index_, std::move(key), std::move(value), &new_index
            );

            auto ret = new_index > -1;
            if(new_index > -1 && !is_black) {
                /* Parent was red! rebalance! */
                _insert_repair_tree(new_index);

                /* Reset root index by walking up to the root */
                int32_t idx = new_index;
                while(data_nodes_[idx].parent_index_ != -1) {
                    idx = data_nodes_[idx].parent_index_;
                }
                root_index_ = idx;

                ret = true;
            }

            assert(leftmost_index_ > -1);

            /* If we inserted, and the parent is the leftmost index
             * then we check to see if this is now the leftmost index */
            if(new_index > -1 && data_nodes_[new_index].parent_index_ == leftmost_index_) {
                if(search_nodes_[leftmost_index_].left_index_ == new_index) {
                    leftmost_index_ = new_index;
                }
            }

            return ret;
        }
    }

    inline int32_t _new_node(int32_t parent_index, const K& key, const V& value) {
        auto ret = (int32_t) search_nodes_.size();

        /* Coordinate growth of both vectors to avoid separate
         * reallocation events — halves the amortised alloc cost
         * compared to letting each vector grow independently. */
        if(ret >= (int32_t)search_nodes_.capacity()) {
            auto new_cap = search_nodes_.capacity() ? search_nodes_.capacity() * 2 : 8;
            search_nodes_.reserve(new_cap);
            data_nodes_.reserve(new_cap);
        }

        search_nodes_.emplace_back(key);

        data_nodes_.emplace_back(key, value);
        data_nodes_.back().parent_index_ = parent_index;

        return ret;
    }

    /* Iterative insertion — replaces the original recursive version
     * to avoid stack frame overhead on SH4. Returns parent_is_black. */
    bool _insert_iterative(int32_t root_index, K&& key, V&& value, int32_t* new_index) {
        assert(root_index > -1);

        int32_t current_index = root_index;

        while(true) {
            auto order = compare_(key, search_nodes_[current_index].key);

            if(order == ThreeWayCompare<K>::RESULT_EQUAL) {
                /* We're inserting a duplicate, so we use the equal_index_ */

                auto new_idx = _new_node(-1, key, value);
                /* _new_node could realloc, so don't hold references across it */

                /* Insert at end of equal-value list for stable ordering */
                auto& root_d = data_nodes_[current_index];
                if(root_d.last_equal_index_ > -1) {
                    data_nodes_[root_d.last_equal_index_].equal_index_ = new_idx;
                } else {
                    root_d.equal_index_ = new_idx;
                }
                root_d.last_equal_index_ = new_idx;

                if(new_index) {
                    *new_index = new_idx;
                }

                /* Node was added to a linked list not the tree itself,
                 * so report parent as black to skip rebalancing */
                return true;

            } else if(order == ThreeWayCompare<K>::RESULT_LESS) {
                int32_t left = search_nodes_[current_index].left_index_;
                if(left == -1) {
                    auto new_idx = _new_node(current_index, key, value);
                    /* Reindex after potential reallocation */
                    search_nodes_[current_index].left_index_ = new_idx;

                    if(new_index) {
                        *new_index = new_idx;
                    }

                    return data_nodes_[current_index].is_black;
                }
                current_index = left;
            } else {
                int32_t right = search_nodes_[current_index].right_index_;
                if(right == -1) {
                    auto new_idx = _new_node(current_index, key, value);
                    /* Reindex after potential reallocation */
                    search_nodes_[current_index].right_index_ = new_idx;

                    if(new_index) {
                        *new_index = new_idx;
                    }

                    return data_nodes_[current_index].is_black;
                }
                current_index = right;
            }
        }
    }

    std::vector<search_node_type> search_nodes_;
    std::vector<data_node_type> data_nodes_;

    int32_t root_index_ = -1;
    int32_t leftmost_index_ = -1;

    Compare compare_;
};


template<typename K, typename V, typename Compare=ThreeWayCompare<K>>
class ContiguousMap {
public:
    typedef typename ContiguousMultiMap<K, V, Compare>::iterator iterator;
    typedef typename ContiguousMultiMap<K, V, Compare>::const_iterator const_iterator;

    ContiguousMap() = default;
    ContiguousMap(std::size_t reserve):
        map_(reserve) {}

    bool empty() const {
        return map_.empty();
    }

    std::size_t size() const {
        return map_.size();
    }

    std::size_t count(const K& key) const {
        return find(key) != end() ? 1 : 0;
    }

    bool insert(const std::pair<K, V>& pair) {
        return insert(pair.first, pair.second);
    }

    bool insert(const K& key, V&& element) {
        if(map_.find(key) == map_.end()) {
            map_.insert(key, std::move(element));
            return true;
        } else {
            return false;
        }
    }

    V& at(const K& key) {
        auto it = map_.find(key);

        if(it == map_.end()) {
            throw std::out_of_range("Key not found");
        }

        return (*it).second;
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

    iterator find(const K& key) {
        return map_.find(key);
    }

    const_iterator find(const K &key) const {
        return map_.find(key);
    }

    iterator begin() {
        return map_.begin();
    }

    iterator end() {
        return map_.end();
    }

    const_iterator begin() const {
        return map_.begin();
    }

    const_iterator end() const {
        return map_.end();
    }
private:
    ContiguousMultiMap<K, V, Compare> map_;
};


}
