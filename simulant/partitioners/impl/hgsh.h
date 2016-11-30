#pragma once

#include <cstring>
#include <map>
#include <unordered_set>
#include "../../interfaces.h"

/*
 * Hierarchical Grid Spatial Hash implementation
 *
 * Objects are inserted into (preferably) one bucket
 */

namespace smlt {


const uint32_t MAX_GRID_LEVELS = 16;


/*
 * Heirarchical hash key. Each level in the key is a hash of cell_size, x, y, z
 * if a child key is visible then all parent and child keys are visible. Using a multimap
 * we can rapidly gather child key objects (by iterating until the key no longer starts with this one)
 * and we can gather parent ones by checking all levels of the hash_path
 */
struct Key {
    std::size_t hash_path[MAX_GRID_LEVELS] = {0};
    std::size_t ancestors = 0;

    bool operator<(const Key& other) const {
        for(std::size_t i = 0; i < MAX_GRID_LEVELS; ++i) {
            if(hash_path[i] > other.hash_path[i]) {
                return false;
            } else if(hash_path[i] < other.hash_path[i]) {
                return true;
            }
        }

        return false; // All equal
    }

    bool operator==(const Key& other) const {
        return (
            ancestors == other.ancestors &&
            memcmp(hash_path, other.hash_path, sizeof(std::size_t) * MAX_GRID_LEVELS) == 0
        );
    }

    bool is_root() const { return ancestors == 0; }
    Key parent_key() const;

    bool is_ancestor_of(const Key& other) const;
};

Key make_key(int32_t cell_size, float x, float y, float z);
std::size_t make_hash(int32_t cell_size, float x, float y, float z);

typedef std::vector<Key> KeyList;

class HGSHEntry {
public:
    virtual ~HGSHEntry() {}


    void push_key(const Key& key) {
        keys_.push_back(key);
    }

    void set_keys(const KeyList& keys) {
        keys_ = keys;
    }

    KeyList keys() const {
        return keys_;
    }

private:
    KeyList keys_;

};

typedef std::unordered_set<HGSHEntry*> HGSHEntryList;

class HGSH {
public:
    HGSH();

    void insert_object_for_box(const AABB& box, HGSHEntry* object);
    void remove_object(HGSHEntry* object);

    HGSHEntryList find_objects_within_box(const AABB& box);
    HGSHEntryList find_objects_within_frustum(const Frustum& frustum);

private:
    int32_t find_cell_size_for_box(const AABB& box) const;
    void insert_object_for_key(Key key, HGSHEntry* entry);

    typedef std::map<Key, std::unordered_set<HGSHEntry*>> Index;
    Index index_;
};

}

