#pragma once

#include <cstring>
#include <map>
#include <set>
#include <ostream>
#include <unordered_set>
#include "../../interfaces.h"

/*
 * Hierarchical Grid Spatial Hash implementation
 *
 * Objects are inserted into (preferably) one bucket
 */

namespace smlt {


const uint32_t MAX_GRID_LEVELS = 16;
struct Hash {
    int16_t x;
    int16_t y;
    int16_t z;
};

/*
 * Heirarchical hash key. Each level in the key is a hash of cell_size, x, y, z
 * if a child key is visible then all parent and child keys are visible. Using a multimap
 * we can rapidly gather child key objects (by iterating until the key no longer starts with this one)
 * and we can gather parent ones by checking all levels of the hash_path
 */
struct Key {
    Hash hash_path[MAX_GRID_LEVELS] = {0};
    std::size_t ancestors = 0;

    bool operator<(const Key& other) const {
        auto len = std::min(other.ancestors, ancestors) + 1;
        auto ret = memcmp(hash_path, other.hash_path, sizeof(Hash) * len);
        return ret < 0 || (ret == 0 && ancestors < other.ancestors);
    }

    bool operator==(const Key& other) const {
        return (
            ancestors == other.ancestors &&
            memcmp(hash_path, other.hash_path, sizeof(Hash) * (ancestors + 1)) == 0
        );
    }

    bool is_root() const { return ancestors == 0; }
    Key parent_key() const;

    bool is_ancestor_of(const Key& other) const;

    friend std::ostream& operator<<(std::ostream& os, const Key& key);
};

std::ostream& operator<<(std::ostream& os, const Key& key);

Key make_key(int32_t cell_size, float x, float y, float z);
Hash make_hash(int32_t cell_size, float x, float y, float z);

typedef std::set<Key> KeyList;

class SpatialHashEntry {
public:
    virtual ~SpatialHashEntry() {}


    void push_key(const Key& key) {
        keys_.insert(key);
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

typedef std::unordered_set<SpatialHashEntry*> HGSHEntryList;

class SpatialHash {
public:
    SpatialHash();

    void insert_object_for_box(const AABB& box, SpatialHashEntry* object);
    void remove_object(SpatialHashEntry* object);

    void update_object_for_box(const AABB& new_box, SpatialHashEntry* object);

    HGSHEntryList find_objects_within_box(const AABB& box);
    HGSHEntryList find_objects_within_frustum(const Frustum& frustum);

private:
    void erase_object_from_key(Key key, SpatialHashEntry* object);

    int32_t find_cell_size_for_box(const AABB& box) const;
    void insert_object_for_key(Key key, SpatialHashEntry* entry);

    typedef std::map<Key, std::unordered_set<SpatialHashEntry*>> Index;
    Index index_;
};

void generate_boxes_for_frustum(const Frustum& frustum, std::vector<AABB>& results);

}

