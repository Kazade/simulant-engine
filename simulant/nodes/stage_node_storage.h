#pragma once

#include "../core/aligned_vector.h"

namespace smlt {

/*
 * StageNodes are the core of the engine, and they
 * cause a lot of problems performance wise.
 *
 * StageNodes form a tree where each child can have unlimited
 * children. StageNodes can also be very different sizes and
 * as users can register their own stage nodes there's no way
 * to know what the "max size" is. This is terrible for
 * cache-locality as each node is dynamically allocated and iterating
 * the tree (which we must do before rendering) can be very slow.
 *
 * This class aims to improve the cache locality of nodes by keeping
 * nodes of similar sizes in contiguous memory. It will try to balance
 * increased locality with bloating memory usage.
 *
 * We try to keep memory 32 byte aligned. On most platforms this aids
 * in performance.
 */

class StageNodeStorage {
public:
    // The alignment of buffer addresses
    constexpr static int alignment = 32;

    // Allocation sizes are rounded up to this to find a buffer
    // to insert into. Must be a multiple of alignment.
    constexpr static int round_up_bytes = 64;

    void* allocate(std::size_t size, std::size_t alignment) {
        if(this->alignment % alignment != 0) {
            S_ERROR("Couldn't fulfil alignment requirements");
            return nullptr;
        }

        // Round the size up to the nearest `round_up_bytes`
        std::size_t rounded_size =
            ((size + round_up_bytes - 1) / round_up_bytes) * round_up_bytes;

        // Look for any free allocations that will hold this size
        // FIXME: If there's a larger free slot, is it worth using?
        // I'm not sure... we're approaching implementing malloc at that point
        for(std::size_t i = 0; i < free_allocations_.size(); ++i) {
            auto& alloc = free_allocations_[i];
            if(alloc.size == rounded_size) {
                // Reuse the existing allocation
                void* ptr = buffers_[rounded_size].data() + alloc.offset;
                used_allocations_[ptr] = alloc;
                std::swap(free_allocations_[i], free_allocations_.back());
                free_allocations_.pop_back();
                return ptr;
            }
        }

        // No existing allocation, create a new one
        auto& buffer = buffers_[rounded_size];
        std::size_t offset = buffer.size();
        buffer.resize(offset + rounded_size);
        void* ptr = buffer.data() + offset;
        used_allocations_[ptr] = {offset, rounded_size};
        max_allocations_[rounded_size] = offset + rounded_size;
        return ptr;
    }

    void deallocate(void* ptr) {
        auto it = used_allocations_.find(ptr);
        if(it == used_allocations_.end()) {
            S_ERROR("Attempted to deallocate unknown pointer");
            return;
        }

        auto alloc = it->second;
        free_allocations_.push_back(alloc);
        used_allocations_.erase(it);

        // FIXME: Write test for this
        if(alloc.offset + alloc.size == max_allocations_[alloc.size]) {
            max_allocations_[alloc.size] = alloc.offset;
        }
    }

    void shrink_to_fit() {
        for(auto& p: max_allocations_) {
            buffers_[p.first].resize(p.second);
            buffers_[p.first].shrink_to_fit();
        }
    }

private:
    struct Allocation {
        std::size_t offset;
        std::size_t size;
    };

    std::unordered_map<std::size_t, std::size_t> max_allocations_;
    std::unordered_map<void*, Allocation> used_allocations_;
    aligned_vector<Allocation, alignment> free_allocations_;
    std::unordered_map<std::size_t, std::vector<uint8_t>> buffers_;
};

} // namespace smlt
