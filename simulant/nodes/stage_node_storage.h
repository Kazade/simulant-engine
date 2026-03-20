#pragma once

#include "../core/aligned_vector.h"

namespace smlt {

/**
 * @class SlabArray
 * @brief A memory pool allocator using slab allocation with bitmap tracking.
 * 
 * Manages allocation and deallocation of fixed-size memory blocks organized into
 * slabs. Each slab contains 16 blocks (one bit per block in a uint16_t bitmap).
 * When a slab becomes fully deallocated, it is freed back to the system.
 * 
 * @tparam alignment The byte alignment requirement for allocated memory blocks.
 *                   Must be satisfied by aligned_alloc().
 * 
 * @note This class is move-only (copy constructor and assignment are deleted).
 * @note Non-thread-safe. External synchronization required for concurrent access.
 */
template<int alignment>
class SlabArray {
public:
    /**
     * @brief Constructs a SlabArray with the specified block size.
     * 
     * @param block_size The size in bytes of each memory block. Multiple blocks
     *                   are packed into each slab.
     */
    SlabArray(std::size_t block_size) :
        block_size_(block_size) {}

    SlabArray(const SlabArray& other) = delete;
    SlabArray& operator=(const SlabArray& other) = delete;

    SlabArray(SlabArray&& other) noexcept = default;
    SlabArray& operator=(SlabArray&& other) noexcept = default;

    /**
     * @brief Allocates a single memory block from the pool.
     * 
     * Searches existing slabs for an available block. If no free block is found,
     * creates a new slab with 16 blocks and returns a pointer to the first block.
     * 
     * @return Pointer to an aligned memory block of size block_size, or nullptr
     *         if allocation fails.
     * 
     * @post The returned pointer remains valid until deallocate() is called on it.
     */
    uint8_t* allocate() {
        for(auto& slab: slabs_) {
            if(slab.used == Slab::full_mask) {
                continue;
            }

            for(std::size_t i = 0; i < Slab::bits; ++i) {
                if((slab.used & (1 << i)) == 0) {
                    slab.used |= (1 << i);
                    return slab.data + block_size_ * i;
                }
            }
        }

        // Create new slab
        uint8_t* data =
            (uint8_t*)aligned_alloc(alignment, block_size_ * Slab::bits);
        assert(data);

        slabs_.push_back(Slab(data));
        slabs_.back().used |= 1;
        return data;
    }

    /**
     * @brief Deallocates a previously allocated memory block.
     * 
     * Marks the block as free within its slab. If the slab becomes completely
     * empty after deallocation, it is freed back to the system and removed from
     * the pool.
     * 
     * @param ptr Pointer to a block previously returned by allocate(). Must not
     *            be null and must belong to this pool.
     * 
     * @return true if the block was successfully deallocated, false if the pointer
     *         was not found in this pool or was already deallocated.
     * 
     * @note Behavior is undefined if ptr is invalid or from a different allocator.
     */
    bool deallocate(uint8_t* ptr) {
        for(auto& slab: slabs_) {
            if(slab.data <= ptr && ptr < slab.data + block_size_ * Slab::bits) {
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
    /**
     * @brief Removes empty slabs from the end of the slab list.
     * 
     * Called after deallocation to reclaim memory. Only removes completely
     * empty slabs from the back of the list to maintain contiguity of
     * active allocations.
     */
    void clean_up() {
        while(!slabs_.empty() && slabs_.back().used == 0) {
            aligned_free(slabs_.back().data);
            slabs_.pop_back();
        }
    }

    /**
     * @struct Slab
     * @brief Represents a single slab containing up to 16 blocks.
     * 
     * @var used Bitmap where each bit represents whether the corresponding block
     *           is allocated (1) or free (0).
     * @var data Pointer to the start of the slab's allocated memory.
     * @var bits Number of allocatable blocks per slab (always 16 for uint16_t).
     * @var full_mask Bitmask with all block bits set (0xFFFF).
     */
    struct Slab {
        uint16_t used = 0;
        uint8_t* data = nullptr;

        static constexpr std::size_t bits = sizeof(used) * 8;
        static constexpr decltype(used) full_mask = ~(decltype(used))0;

        /**
         * @brief Constructs a Slab with the given memory allocation.
         * 
         * @param data Pointer to pre-allocated aligned memory for this slab.
         */
        Slab(uint8_t* data) :
            data(data) {}
    };

    std::size_t block_size_;  ///< Size in bytes of each allocated block
    std::vector<Slab> slabs_; ///< Collection of active slabs
};

/**
 * @class StageNodeStorage
 * @brief A hierarchical memory pool allocator optimized for StageNode allocation.
 * 
 * Improves cache locality when allocating StageNodes of varying sizes by:
 * - Grouping allocations by rounded-up size into separate pools
 * - Maintaining each pool with slab-based allocation
 * - Using aligned allocation (32-byte alignment) for better CPU cache performance
 * 
 * StageNodes can be arbitrary sizes (user-defined subclasses), so this allocator
 * rounds up requested sizes to multiples of 256 bytes and maintains separate
 * SlabArray pools for each size class. This balances memory locality gains
 * against memory fragmentation.
 * 
 * @note Thread-safety: Not thread-safe. Requires external synchronization for
 *       concurrent allocation/deallocation.
 * 
 * @see SlabArray
 */
class StageNodeStorage {
public:
    /// @brief The byte alignment guarantee for all allocated memory (32 bytes).
    constexpr static std::size_t node_alignment = 32;

    /// @brief Size class granularity. Allocation sizes are rounded up to multiples
    ///        of this value (256 bytes). Must be a multiple of node_alignment.
    constexpr static std::size_t round_up_bytes = 256;

    /**
     * @brief Allocates a block of memory with the specified size and alignment.
     * 
     * The requested size is rounded up to the nearest multiple of round_up_bytes,
     * and the allocation is drawn from the pool for that size class. If no pool
     * exists for that size, one is created.
     * 
     * @param size The minimum number of bytes to allocate.
     * @param alignment The required byte alignment. Must be a divisor of node_alignment
     *                  (i.e., alignment % node_alignment must equal 0).
     * 
     * @return A pointer to allocated memory of at least @a size bytes with the
     *         requested alignment, or nullptr if the alignment requirement cannot
     *         be satisfied.
     * 
     * @post The returned pointer must be deallocated with deallocate() on this
     *       same StageNodeStorage instance.
     * 
     * @note Allocations are tracked internally by pointer address.
     */
    void* allocate(std::size_t size, std::size_t alignment) {
        if(alignment % node_alignment != 0) {
            S_ERROR("Couldn't fulfil alignment requirements");
            return nullptr;
        }

        // Round the size up to the nearest `round_up_bytes`
        std::size_t rounded_size =
            ((size + round_up_bytes - 1) / round_up_bytes) * round_up_bytes;

        if(!buffers_.count(rounded_size)) {
            buffers_.insert(std::make_pair(
                rounded_size, SlabArray<node_alignment>(rounded_size)));
        }

        void* ptr = buffers_.at(rounded_size).allocate();
        used_allocations_[ptr] = rounded_size;
        return ptr;
    }

    /**
     * @brief Deallocates a previously allocated memory block.
     * 
     * Returns the block to its size-class pool and cleans up empty slabs.
     * 
     * @param ptr Pointer previously returned by allocate() on this instance.
     *            Must not be null.
     * 
     * @pre @a ptr must have been returned by a previous call to allocate() on
     *      this same StageNodeStorage instance and not already deallocated.
     * 
     * @note Behavior is undefined if @a ptr is invalid or from a different allocator.
     * 
     * @see allocate()
     */
    void deallocate(void* ptr) {
        auto it = used_allocations_.find(ptr);
        if(it == used_allocations_.end()) {
            S_ERROR("Attempted to deallocate unknown pointer");
            return;
        }

        if(!buffers_.at(it->second).deallocate((uint8_t*)ptr)) {
            return;
        }

        used_allocations_.erase(it);
    }

private:
    /// @brief Maps allocated pointers to their size class for O(1) lookup during deallocation.
    std::unordered_map<void*, std::size_t> used_allocations_;
    
    /// @brief Maps size classes (in bytes) to their respective slab allocation pools.
    std::unordered_map<std::size_t, SlabArray<32>> buffers_;
};

} // namespace smlt
