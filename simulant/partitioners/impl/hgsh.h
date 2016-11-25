#pragma once

#include <unordered_set>
#include "../../interfaces.h"

/*
 * Hierarchical Grid Spatial Hash implementation
 *
 * Objects are inserted into (preferably) one bucket
 */

namespace smlt {


typedef int32_t Hash;

struct BucketHash {
    float cell_size;
    Hash hash;
};

typedef std::vector<BucketHash> BucketHashList;


class HGSHEntry {
public:
    virtual ~HGSHEntry() {}


    void push_bucket(const BucketHash& hash) {
        buckets_.push_back(hash);
    }

    void set_buckets(const BucketHashList buckets) {
        buckets_ = buckets;
    }

    BucketHashList buckets() const {
        return buckets_;
    }

private:
    BucketHashList buckets_;

};

typedef std::unordered_set<HGSHEntry*> HGSHEntryList;

class HGSH {
public:
    HGSH(float min_cell_size = 256.0, float max_cell_size = 32768.0);

    void insert_object_for_box(const AABB& box, HGSHEntry* object);
    void remove_object(HGSHEntry* object);

    HGSHEntryList find_objects_within_box(const AABB& box);
    HGSHEntryList find_objects_within_frustum(const Frustum& frustum);

    uint32_t grid_count() const { return buckets_.size(); }

private:
    Hash make_key(int32_t x, int32_t y, int32_t z) {
        return (x * 73856093) ^ (y * 19349663) ^ (z * 83492791);
    }

    float find_cell_size_for_box(AABB box) const;

    void insert_object_for_hash(float cell_size, Hash hash, HGSHEntry* entry);

    struct Bucket {
        HGSHEntryList objects;
    };


    typedef std::unordered_map<Hash, std::shared_ptr<Bucket>> HashBucket;
    std::unordered_map<float, HashBucket> buckets_;

    float min_size_ = 256.0f;
    float max_size_ = 1024.0f;
};

}

