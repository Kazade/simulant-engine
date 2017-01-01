#include <cmath>
#include "../../frustum.h"
#include "spatial_hash.h"
#include "../../utils/endian.h"

namespace smlt {

SpatialHash::SpatialHash() {

}


void SpatialHash::insert_object_for_box(const AABB &box, SpatialHashEntry *object) {
    auto cell_size = find_cell_size_for_box(box);

    for(auto& corner: box.corners()) {
        auto key = make_key(cell_size, corner.x, corner.y, corner.z);
        insert_object_for_key(key, object);
    }
}

void SpatialHash::remove_object(SpatialHashEntry *object) {
    for(auto key: object->keys()) {
        erase_object_from_key(key, object);
    }
    object->set_keys(KeyList());
}

void SpatialHash::erase_object_from_key(Key key, SpatialHashEntry* object) {
    auto it = index_.find(key);
    if(it != index_.end()) {
        it->second.erase(object);

        if(it->second.empty()) {
            index_.erase(it);
        }
    }
}

void SpatialHash::update_object_for_box(const AABB& new_box, SpatialHashEntry* object) {
    auto cell_size = find_cell_size_for_box(new_box);

    KeyList new_keys;
    KeyList old_keys = object->keys();

    for(auto& corner: new_box.corners()) {
        auto key = make_key(cell_size, corner.x, corner.y, corner.z);
        new_keys.insert(key);
    }


    KeyList keys_to_add, keys_to_remove;

    std::set_difference(
        new_keys.begin(), new_keys.end(), old_keys.begin(), old_keys.end(),
        std::inserter(keys_to_add, keys_to_add.end())
    );

    std::set_difference(
        old_keys.begin(), old_keys.end(), new_keys.begin(), new_keys.end(),
        std::inserter(keys_to_remove, keys_to_remove.end())
    );

    if(new_keys.empty() && old_keys.empty()) {
        return;
    }

    for(auto& key: keys_to_remove) {
        erase_object_from_key(key, object);
    }

    for(auto& key: keys_to_add) {
        insert_object_for_key(key, object);
    }

    object->set_keys(new_keys);
}

void generate_boxes_for_frustum(const Frustum& frustum, std::vector<AABB>& results) {
    results.clear(); // Required

    // start at the center of the near plane
    Vec3 start_point = Vec3::find_average(frustum.far_corners());

    // We want to head in the reverse direction of the frustum
    Vec3 direction = -frustum.direction().normalized();

    // Get the normals of the up and right planes so we can generated boxes
    Vec3 up = frustum.plane(FRUSTUM_PLANE_BOTTOM).normal();
    Vec3 right = frustum.plane(FRUSTUM_PLANE_LEFT).normal();

    auto near_plane = frustum.plane(FRUSTUM_PLANE_NEAR);

    // Project the up and right normals onto the near plane (otherwise they might be skewed)
    up = near_plane.project(up).normalized();
    right = near_plane.project(right).normalized();

    float distance_left = frustum.depth();
    auto p = start_point;
    while(distance_left > 0) {
        float box_size = std::max(frustum.width_at_distance(distance_left), frustum.height_at_distance(distance_left));
        float hw = box_size / 2.0;

        std::array<Vec3, 8> corners;

        corners[0] = p - (direction * box_size) - (right * hw) - (up * hw);
        corners[1] = p - (direction * box_size) + (right * hw) - (up * hw);
        corners[2] = p - (direction * box_size) + (right * hw) + (up * hw);
        corners[3] = p - (direction * box_size) - (right * hw) + (up * hw);

        corners[4] = p - (right * hw) - (up * hw);
        corners[5] = p + (right * hw) - (up * hw);
        corners[6] = p + (right * hw) + (up * hw);
        corners[7] = p - (right * hw) + (up * hw);


        AABB new_box(corners.data(), corners.size());
        results.push_back(new_box);

        distance_left -= box_size;
        p += direction * box_size;
    }
}

HGSHEntryList SpatialHash::find_objects_within_frustum(const Frustum &frustum) {
    static std::vector<AABB> boxes; // Static to avoid repeated allocations

    generate_boxes_for_frustum(frustum, boxes);

    HGSHEntryList results;

    for(auto& box: boxes) {
        auto ret = find_objects_within_box(box);
        if(!ret.empty()) {
            results.insert(ret.begin(), ret.end());
        }
    }

    return results;
}

HGSHEntryList SpatialHash::find_objects_within_box(const AABB &box) {
    HGSHEntryList objects;

    auto cell_size = find_cell_size_for_box(box);

    auto gather_objects = [](Index& index, const Key& key, HGSHEntryList& objects) {
        auto it = index.lower_bound(key);
        if(it == index.end()) {
            return;
        }

        // First, iterate the index to find a key which isn't a descendent of this one
        // then break
        while(it != index.end() && key.is_ancestor_of(it->first)) {
            objects.insert(it->second.begin(), it->second.end());
            ++it;
        }

        // Now, go up the tree looking for objects which are ancestors of this key
        auto path = key;
        while(!path.is_root()) {
            path = path.parent_key();
            it = index.find(path);
            if(it != index.end() && path.is_ancestor_of(it->first)) {
                objects.insert(it->second.begin(), it->second.end());
            }
        }
    };

    auto corners = box.corners();

    std::set<Key> seen;

    for(auto& corner: corners) {
        auto key = make_key(
            cell_size,
            corner.x,
            corner.y,
            corner.z
        );

        // Don't look at the same key more than once
        // FIXME: implement a hashing function and use unordered_set
        if(seen.find(key) == seen.end()) {
            gather_objects(index_, key, objects);
            seen.insert(key);
        }
    }


    return objects;
}

int32_t SpatialHash::find_cell_size_for_box(const AABB &box) const {
    /*
     * We find the nearest hash size which is greater than double the max dimension of the
     * box. This increases the likelyhood that the object will not wastefully span cells
     */

    auto maxd = box.max_dimension();
    if(maxd < 1.0f) {
        return 1;
    } else {
        return 1 << uint32_t(std::ceil(std::log2(maxd)));
    }
}

void SpatialHash::insert_object_for_key(Key key, SpatialHashEntry *entry) {
    auto it = index_.find(key);
    if(it != index_.end()) {
        it->second.insert(entry);
    } else {
        HGSHEntryList list;
        list.insert(entry);
        index_.insert(std::make_pair(key, list));
    }
    entry->push_key(key);
}

Key make_key(int32_t cell_size, float x, float y, float z) {
    int32_t path_size = pow(2, MAX_GRID_LEVELS - 1); // Minus 1, because cell_size 1 is not a power of 2
    Key key;

    uint32_t ancestor_count = 0;

    while(path_size > cell_size) {
        assert(ancestor_count < MAX_GRID_LEVELS);

        key.hash_path[ancestor_count] = make_hash(path_size, x, y, z);
        path_size /= 2;
        ancestor_count++;
    }

    assert(ancestor_count < MAX_GRID_LEVELS);

    key.hash_path[ancestor_count] = make_hash(cell_size, x, y, z);
    key.ancestors = ancestor_count;

    return key;
}

Hash make_hash(int32_t cell_size, float x, float y, float z) {
    Hash hash;

    hash.x = ensure_big_endian(int16_t(std::floor(x / cell_size)));
    hash.y = ensure_big_endian(int16_t(std::floor(y / cell_size)));
    hash.z = ensure_big_endian(int16_t(std::floor(z / cell_size)));

    return hash;
}

Key Key::parent_key() const {
    assert(!is_root());

    Key ret;
    memcpy(ret.hash_path, hash_path, sizeof(Hash) * ancestors);
    ret.ancestors = ancestors - 1;
    return ret;
}

bool Key::is_ancestor_of(const Key &other) const {
    if(ancestors > other.ancestors) return false;

    return memcmp(hash_path, other.hash_path, sizeof(Hash) * (ancestors + 1)) == 0;
}

std::ostream &operator<<(std::ostream &os, const Key &key) {
    for(uint32_t i = 0; i < key.ancestors + 1; ++i) {
        os << key.hash_path[i].x << key.hash_path[i].y << key.hash_path[i].z;
        if(i != key.ancestors) {
            os << " / ";
        }
    }

    return os;
}


}
