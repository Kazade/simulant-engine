#pragma once

#include "../core/aligned_vector.h"

namespace smlt {

template<int alignment>
class SlabArray {
public:
    SlabArray(std::size_t block_size) :
        block_size_(block_size) {}

    uint8_t* allocate() {
        for(auto& slab: slabs_) {
            if(slab.used == 0xFFFF) {
                continue;
            }

            for(std::size_t i = 0; i < 16; ++i) {
                if((slab.used & (1 << i)) == 0) {
                    slab.used |= (1 << i);
                    return slab.data + block_size_ * i;
                }
            }
        }

        // Create new slab
        uint8_t* data = (uint8_t*)aligned_alloc(alignment, block_size_ * 16);
        assert(data);

        slabs_.push_back(Slab(data));
        slabs_.back().used |= 1;
        return data;
    }

    bool deallocate(uint8_t* ptr) {
        for(auto& slab: slabs_) {
            if(slab.data <= ptr && ptr < slab.data + block_size_ * 16) {
                std::size_t offset = ptr - slab.data;
                std::size_t index = offset / block_size_;
                if(slab.used & (1 << index)) {
                    slab.used &= ~(1 << index);
                    clean_up();
                    return true;
                }
            }
        }

        S_ERROR("Attempted to deallocate a pointer which was not "
                "allocated");
        return false;
    }

private:
    void clean_up() {
        while(!slabs_.empty() && slabs_.back().used == 0) {
            aligned_free(slabs_.back().data);
            slabs_.pop_back();
        }
    }

    struct Slab {
        uint16_t used = 0;
        uint8_t* data = nullptr;

        Slab(uint8_t* data) :
            data(data) {}
    };

    std::size_t block_size_;
    std::vector<Slab> slabs_;
};

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
    constexpr static int round_up_bytes = 256;

    void* allocate(std::size_t size, std::size_t alignment) {
        if(this->alignment % alignment != 0) {
            S_ERROR("Couldn't fulfil alignment requirements");
            return nullptr;
        }

        // Round the size up to the nearest `round_up_bytes`
        std::size_t rounded_size =
            ((size + round_up_bytes - 1) / round_up_bytes) * round_up_bytes;

        if(!buffers_.count(rounded_size)) {
            buffers_.insert(std::make_pair(
                rounded_size, SlabArray<this->alignment>(rounded_size)));
        }

        void* ptr = buffers_.at(rounded_size).allocate();
        used_allocations_[ptr] = rounded_size;
        return ptr;
    }

    void deallocate(void* ptr) {
        auto it = used_allocations_.find(ptr);
        if(it == used_allocations_.end()) {
            S_ERROR("Attempted to deallocate unknown pointer");
            return;
        }

        buffers_.at(it->second).deallocate((uint8_t*)ptr);
        used_allocations_.erase(it);
    }

private:
    std::unordered_map<void*, std::size_t> used_allocations_;
    std::unordered_map<std::size_t, SlabArray<32>> buffers_;
};

} // namespace smlt
