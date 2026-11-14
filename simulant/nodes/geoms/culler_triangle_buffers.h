#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../assets/material.h"
#include "../../types.h"
#include "../../vertex_data.h"

namespace smlt {

/* Triangles for a single node, grouped by material.
 *
 * During compilation the indices are collected in `pending`. Once the tree is
 * built, CullerMaterialBuffers moves them into one shared index buffer per
 * material and fills in `indexes`, `first_index` and `count`.
 */
struct CullerNodeTriangles {
    struct Entry {
        Material* material = nullptr;
        std::vector<uint32_t> pending;

        IndexData::ptr indexes;
        uint32_t first_index = 0;
        uint32_t count = 0;
    };

    std::vector<Entry> entries;

    Entry& entry_for(Material* material) {
        for(auto& entry: entries) {
            if(entry.material == material) {
                return entry;
            }
        }

        entries.emplace_back();
        entries.back().material = material;
        return entries.back();
    }
};

/* Stores the triangles of all nodes in one index buffer per material.
 *
 * Each node's triangles are appended consecutively in node traversal order.
 * Keeping the buffers shared means that the number of GPU buffers is limited
 * to the number of materials, rather than one buffer per node/material, and
 * lets adjacent nodes be merged into a single draw call at render time.
 */
class CullerMaterialBuffers {
public:
    CullerMaterialBuffers(IndexType index_type):
        index_type_(index_type) {}

    void add(CullerNodeTriangles::Entry& entry) {
        auto it = buffers_.find(entry.material->id());
        if(it == buffers_.end()) {
            it = buffers_.emplace(
                entry.material->id(),
                Buffer{std::make_shared<IndexData>(index_type_)}).first;
        }

        auto& buffer = it->second;
        entry.indexes = buffer.indexes;
        entry.first_index = buffer.indexes->count();
        entry.count = static_cast<uint32_t>(entry.pending.size());

        if(!entry.pending.empty()) {
            buffer.indexes->index(entry.pending.data(), entry.pending.size());
        }

        // The temporary storage is no longer needed
        std::vector<uint32_t>().swap(entry.pending);
    }

    void done() {
        for(auto& p: buffers_) {
            p.second.indexes->done();
        }
    }

private:
    struct Buffer {
        IndexData::ptr indexes;
    };

    IndexType index_type_;
    std::unordered_map<AssetID, Buffer> buffers_;
};

/* Combines adjacent ranges belonging to the same material into a single
 * renderable. Ranges must be supplied in node traversal order, matching the
 * order used by CullerMaterialBuffers.
 */
class CullerRangeAccumulator {
public:
    typedef std::function<void (Material*, IndexData*, uint32_t, uint32_t)>
        FlushCallback;

    CullerRangeAccumulator(FlushCallback flush):
        flush_(std::move(flush)) {}

    void add(Material* material, IndexData* indexes, uint32_t first_index,
             uint32_t count) {
        if(!count) {
            return;
        }

        auto id = material->id();
        auto it = runs_.find(id);

        if(it != runs_.end() && it->second.indexes == indexes &&
           it->second.first_index + it->second.count == first_index) {
            it->second.count += count;
            return;
        }

        if(it != runs_.end()) {
            flush_(it->second.material, it->second.indexes,
                   it->second.first_index, it->second.count);
        }

        runs_[id] = Run{material, indexes, first_index, count};
    }

    void finish() {
        for(auto& p: runs_) {
            flush_(p.second.material, p.second.indexes,
                   p.second.first_index, p.second.count);
        }
        runs_.clear();
    }

private:
    struct Run {
        Material* material = nullptr;
        IndexData* indexes = nullptr;
        uint32_t first_index = 0;
        uint32_t count = 0;
    };

    FlushCallback flush_;
    std::unordered_map<AssetID, Run> runs_;
};

/* A stable key for a renderable which is a slice of a shared index buffer.
 * Geom geometry is static so the key must only change when the geometry
 * represented by the renderable changes. */
inline int64_t culler_range_key(const IndexData* indexes,
                                uint32_t first_index) {
    uint64_t h = indexes->uuid();
    h ^= uint64_t(first_index) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
    return static_cast<int64_t>(h);
}

} // namespace smlt
