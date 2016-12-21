#include <cmath>
#include "../../frustum.h"
#include "spatial_hash.h"

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
        auto it = index_.find(key);
        if(it != index_.end()) {
            it->second.erase(object);

            if(it->second.empty()) {
                index_.erase(it);
            }
        }
    }
    object->set_keys(KeyList());
}

void generate_boxes_for_frustum(const Frustum& frustum, float box_size, std::vector<AABB>& results) {
    results.clear(); // Required

    // start at the center of the near plane
    Vec3 start_point = Vec3::find_average(frustum.near_corners());

    // We want to head in the direction of the frustum
    Vec3 direction = frustum.direction().normalized();

    // Get the normals of the up and right planes so we can generated boxes
    Vec3 up = frustum.plane(FRUSTUM_PLANE_BOTTOM).normal();
    Vec3 right = frustum.plane(FRUSTUM_PLANE_LEFT).normal();

    // Project the up and right normals onto the near plane (otherwise they might be skewed)
    up = frustum.plane(FRUSTUM_PLANE_NEAR).project(up).normalized();
    right = frustum.plane(FRUSTUM_PLANE_NEAR).project(right).normalized();

    auto slices = std::ceil(frustum.depth() / box_size);

    for(auto i = 0; i < slices; ++i) {
        // Get the width and the height of the frustum at the far edge of this box slice
        float width = frustum.width_at_distance((i + 1) * box_size);
        float height = frustum.height_at_distance((i + 1) * box_size);

        float halfWidth = width * 0.5;
        float halfHeight = height * 0.5;

        // Generate the boxes for this slice
        for(float x = 0; x < halfWidth; x += box_size) {
            for(float y = 0; y < halfHeight; y += box_size) {

                Vec3 min = start_point + (right * x) + (up * y);
                Vec3 max = start_point + (right * (x + box_size)) + (up * (y + box_size)) + (direction * box_size);

                results.push_back(AABB(min, max));

                min = start_point + (right * x) + (-up * y);
                max = start_point + (right * (x + box_size)) + (-up * (y + box_size)) + (direction * box_size);

                results.push_back(AABB(min, max));

                min = start_point + (-right * x) + (-up * y);
                max = start_point + (-right * (x + box_size)) + (-up * (y + box_size)) + (direction * box_size);

                results.push_back(AABB(min, max));

                min = start_point + (-right * x) + (up * y);
                max = start_point + (-right * (x + box_size)) + (up * (y + box_size)) + (direction * box_size);

                results.push_back(AABB(min, max));
            }
        }

        start_point += direction * box_size;
    }
}

HGSHEntryList SpatialHash::find_objects_within_frustum(const Frustum &frustum) {
    auto box_size = frustum.depth() / 5.0;

    static std::vector<AABB> boxes; // Static to avoid repeated allocations

    generate_boxes_for_frustum(frustum, box_size, boxes);

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
        while(true) {
            path = path.parent_key();
            it = index.find(path);
            if(it != index.end() && path.is_ancestor_of(it->first)) {
                objects.insert(it->second.begin(), it->second.end());
            }

            if(path.is_root()) {
                break;
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

    int32_t k = 1;

    while(k < box.max_dimension()) {
        k *= 2;
    }

    return k;
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
    std::size_t seed = 0;

    auto xp = int32_t(std::floor(x / cell_size));
    auto yp = int32_t(std::floor(y / cell_size));
    auto zp = int32_t(std::floor(z / cell_size));

    std::hash_combine(seed, xp);
    std::hash_combine(seed, yp);
    std::hash_combine(seed, zp);

    return seed;
}

Key Key::parent_key() const {
    assert(!is_root());

    Key ret;
    memcpy(ret.hash_path, hash_path, sizeof(std::size_t) * ancestors);
    ret.ancestors = ancestors - 1;
    return ret;
}

bool Key::is_ancestor_of(const Key &other) const {
    if(ancestors > other.ancestors) return false;

    return memcmp(hash_path, other.hash_path, sizeof(std::size_t) * (ancestors + 1)) == 0;
}

std::ostream &operator<<(std::ostream &os, const Key &key) {
    for(std::size_t i = 0; i < key.ancestors + 1; ++i) {
        os << key.hash_path[i];
        if(i != key.ancestors) {
            os << " / ";
        }
    }

    return os;
}


}
