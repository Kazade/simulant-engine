#include <cmath>
#include "../../frustum.h"
#include "hgsh.h"

namespace smlt {

HGSH::HGSH() {

}


void HGSH::insert_object_for_box(const AABB &box, HGSHEntry *object) {
    auto cell_size = find_cell_size_for_box(box);

    auto xmin = int32_t(floor(box.min.x / cell_size));
    auto xmax = int32_t(floor(box.max.x / cell_size));
    auto ymin = int32_t(floor(box.min.y / cell_size));
    auto ymax = int32_t(floor(box.max.y / cell_size));
    auto zmin = int32_t(floor(box.min.z / cell_size));
    auto zmax = int32_t(floor(box.max.z / cell_size));

    for(int32_t x = xmin; x <= xmax; ++x) {
        for(int32_t y = ymin; y <= ymax; ++y) {
            for(int32_t z = zmin; z <= zmax; ++z) {
                auto key = make_key(std::floor(cell_size), x, y, z);
                insert_object_for_key(key, object);
            }
        }
    }
}

void HGSH::remove_object(HGSHEntry *object) {
    for(auto key: object->keys()) {
        auto it = index_.find(key);
        if(it != index_.end()) {
            it->second.erase(object);
        }

        if(it->second.empty()) {
            index_.erase(it);
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

    auto xmin = int32_t(floor(box.min.x / cell_size));
    auto xmax = int32_t(floor(box.max.x / cell_size));
    auto ymin = int32_t(floor(box.min.y / cell_size));
    auto ymax = int32_t(floor(box.max.y / cell_size));
    auto zmin = int32_t(floor(box.min.z / cell_size));
    auto zmax = int32_t(floor(box.max.z / cell_size));

    auto gather_objects = [this, &objects](const Key& key) {
        auto it = index_.find(key);
        if(it == index_.end()) {
            return;
        }

        // First, iterate the index to find a key which isn't a descendent of this one
        // then break
        while(it != index_.end() && key.is_ancestor_of(it->first)) {
            objects.insert(it->second.begin(), it->second.end());
            ++it;
        }

        // Now, go up the tree looking for objects which are ancestors of this key
        auto path = key;
        while(true) {
            path = path.parent_key();
            it = index_.find(path);
            if(it != index_.end()) {
                objects.insert(it->second.begin(), it->second.end());
            }

            if(path.is_root()) {
                break;
            }
        }
    };


    for(int32_t x = xmin; x <= xmax; ++x) {
        for(int32_t y = ymin; y <= ymax; ++y) {
            for(int32_t z = zmin; z <= zmax; ++z) {
                auto key = make_key(std::floor(cell_size), x, y, z);
                gather_objects(key);
            }
        }
    }


    return objects;
}

int32_t HGSH::find_cell_size_for_box(const AABB &box) const {
    /*
     * We find the nearest hash size which is greater than double the max dimension of the
     * box. This increases the likelyhood that the object will not wastefully span cells
     */
    const int32_t MAX_SIZE = pow(1, MAX_GRID_LEVELS);
    int32_t i = 1;

    while(i < std::min(box.max_dimension(), (float) MAX_SIZE)) {
        i *= 2;
    }

    return i;
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
}

Key make_key(int32_t cell_size, float x, float y, float z) {
    int32_t path_size = pow(1, MAX_GRID_LEVELS);
    Key key;

    uint32_t ancestor_count = 0;

    while(path_size > cell_size) {
        key.hash_path[ancestor_count++] = make_hash(path_size, x, y, z);
        path_size /= 2;
    }

    key.hash_path[ancestor_count++] = make_hash(cell_size, x, y, z);
    key.ancestors = ancestor_count;

    return key;
}

std::size_t make_hash(int32_t cell_size, float x, float y, float z) {
    std::size_t seed = 0;
    std::hash_combine(seed, cell_size);
    std::hash_combine(seed, int32_t(x));
    std::hash_combine(seed, int32_t(y));
    std::hash_combine(seed, int32_t(z));

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
    if(ancestors >= other.ancestors) return false;

    return memcmp(hash_path, other.hash_path, sizeof(std::size_t) * ancestors) == 0;
}


}
