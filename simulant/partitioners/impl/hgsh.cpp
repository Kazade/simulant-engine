#include <cmath>
#include "../../frustum.h"
#include "hgsh.h"

namespace smlt {

HGSH::HGSH() {

}


void HGSH::insert_object_for_box(const AABB &box, HGSHEntry *object) {
    auto cell_size = find_cell_size_for_box(box);

    for(auto& corner: box.corners()) {
        auto key = make_key(cell_size, corner.x, corner.y, corner.z);
        insert_object_for_key(key, object);
    }
}

void HGSH::remove_object(HGSHEntry *object) {
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

HGSHEntryList HGSH::find_objects_within_frustum(const Frustum &frustum) {
    // FIXME: This just builds an AABB around the frustum, for perspective
    // projections this likely isn't efficient (fine for ortho)

    const float min = std::numeric_limits<float>::lowest();
    const float max = std::numeric_limits<float>::max();

    AABB box;
    box.min.x = box.min.y = box.min.z = max;
    box.max.x = box.max.y = box.max.z = min;

    auto update_box = [&box](const Vec3& corner) {
        if(corner.x < box.min.x) box.min.x = corner.x;
        if(corner.y < box.min.y) box.min.y = corner.y;
        if(corner.z < box.min.z) box.min.z = corner.z;

        if(corner.x > box.max.x) box.max.x = corner.x;
        if(corner.y > box.max.y) box.max.y = corner.y;
        if(corner.z > box.max.z) box.max.z = corner.z;
    };

    for(auto& corner: frustum.near_corners()) {
        update_box(corner);
    }


    for(auto& corner: frustum.far_corners()) {
        update_box(corner);
    }

    return find_objects_within_box(box);
}

HGSHEntryList HGSH::find_objects_within_box(const AABB &box) {
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

    for(auto& corner: corners) {
        auto key = make_key(
            cell_size,
            corner.x,
            corner.y,
            corner.z
        );
        gather_objects(index_, key, objects);
    }


    return objects;
}

int32_t HGSH::find_cell_size_for_box(const AABB &box) const {
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

void HGSH::insert_object_for_key(Key key, HGSHEntry *entry) {
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
