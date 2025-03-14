#include <cstdint>
#include <memory>
#include <numeric>
#include <set>

namespace smlt {

struct Block {
    std::size_t offset = 0;
    std::size_t size = 0;
    std::size_t used = 0;

    // Free blocks should be ordered by increasing size
    // so that when allocating we find the smallest
    // available first
    bool operator<(const Block& rhs) const {
        return size < rhs.size;
    }

    std::size_t remaining() const {
        return size - used;
    }
};

/*
 * Really basic allocator over a vector that allocates dynamically sized
 * blocks with the specified alignments */
template<int Alignment>
class DynamicAlignedAllocator {

public:
    static constexpr int alignment = Alignment;

    ~DynamicAlignedAllocator() {
        aligned_free(data_);
    }

    uint8_t* allocate(std::size_t size) {
        // We can't iterate the set while manipulating the set
        // so we iterate this instead
        std::vector<Block*> block_ptrs;
        for(auto it = blocks_.begin(); it != blocks_.end(); ++it) {
            block_ptrs.push_back(const_cast<Block*>(&(*it)));
        }

        for(auto block_ptr: block_ptrs) {
            auto rounded_used = round_to_alignment(block_ptr->used);
            auto block_cap = block_ptr->size - rounded_used;

            auto rounded_required_size = round_to_alignment(size);

            if(block_cap >= rounded_required_size) {
                // If this block has been used, then the new block will follow
                // the used portion. If it's not been used, then we'll use it
                // now and the new block will follow what we use

                auto new_block_offset =
                    (block_ptr->used)
                        ? block_ptr->offset + rounded_used
                        : block_ptr->offset + rounded_required_size;

                auto new_block_size =
                    (block_ptr->used) ? block_ptr->size - rounded_used
                                      : block_ptr->size - rounded_required_size;

                auto new_block_used = (block_ptr->used) ? size : 0;

                uint8_t* data = (block_ptr->used) ? data_ + new_block_offset
                                                  : data_ + block_ptr->offset;

                // Split the block
                auto new_block =
                    Block{new_block_offset, new_block_size, new_block_used};

                block_ptr = update_size(block_ptr, -int(new_block_size));
                block_ptr->used = (block_ptr->used) ? block_ptr->used : size;

                // These values should always remain aligned
                assert(round_to_alignment(new_block.offset) ==
                       new_block.offset);
                assert(round_to_alignment(new_block.size) == new_block.size);

                blocks_.insert(new_block);

                remove_zero_sized_free_blocks();

                return data;
            }
        }

        // No free blocks? We need to allocate a new block and then update
        auto additional_bytes = round_to_alignment(size);
        uint8_t* new_data = (uint8_t*)smlt::aligned_alloc(
            alignment, allocated_size_ + additional_bytes);
        auto offset = allocated_size_;
        memcpy(new_data, data_, allocated_size_);
        allocated_size_ = allocated_size_ + additional_bytes;

        Block new_block{offset, additional_bytes, size};

        // These values should always remain aligned
        assert(round_to_alignment(new_block.offset) == new_block.offset);
        assert(round_to_alignment(new_block.size) == new_block.size);
        blocks_.insert(new_block);

        if(realloc_callback_) {
            // Notify the callback so we can update pointers
            realloc_callback_(data_, new_data, user_data_);
        }

        aligned_free(data_);
        data_ = new_data;
        return data_ + offset;
    }

    void deallocate(const uint8_t* ptr) {
        auto it =
            std::find_if(blocks_.begin(), blocks_.end(), [=](const Block& b) {
            return (data_ + b.offset) == ptr;
        });

        assert(it != blocks_.end());
        if(it != blocks_.end()) {
            const_cast<Block&>(*it).used = 0;
        }

        merge_free_blocks();
    }

    void set_realloc_callback(void (*callback)(uint8_t*, uint8_t*, void*),
                              void* user_data) {
        realloc_callback_ = callback;
        user_data_ = user_data;
    }

    std::size_t capacity() const {
        return allocated_size_;
    }

    std::size_t used() const {
        return capacity() - std::accumulate(blocks_.begin(), blocks_.end(), 0,
                                            [](std::size_t n, const Block& b) {
            return (b.size - b.used) + n;
        });
    }

    std::size_t _block_count() const {
        return blocks_.size();
    }

    const Block* _block(std::size_t which) const {
        return &(*(std::next(blocks_.begin(), which)));
    }

private:
    std::size_t round_to_alignment(std::size_t size) {
        return ((size + alignment - 1) / alignment) * alignment;
    }

    uint8_t* data_ = nullptr;
    std::size_t allocated_size_ = 0;
    std::multiset<Block> blocks_;
    void (*realloc_callback_)(uint8_t*, uint8_t*, void*);
    void* user_data_;

    /* You can't update the property of a value in a set that will
     * change the order, so we have to remove, copy + change, reinsert */
    Block* update_size(const Block* block, int change) {
        auto it = std::find_if(blocks_.begin(), blocks_.end(),
                               [block](const Block& b) {
            return &b == block;
        });

        if(it == blocks_.end()) {
            return nullptr;
        }

        auto new_block = *it;
        new_block.size += change;
        blocks_.erase(it);

        // These values should always remain aligned
        assert(round_to_alignment(new_block.offset) == new_block.offset);
        assert(round_to_alignment(new_block.size) == new_block.size);
        it = blocks_.insert(new_block);
        return const_cast<Block*>(&(*it));
    }

    void merge_free_blocks() {
        // Merge consecutive free blocks by first ordering by
        // offset, and then iterating and moving the size to the
        // previous block, finally go through and remove zero-sized
        // blocks
        struct Cmp {
            bool operator()(const Block* lhs, const Block* rhs) const {
                return lhs->offset < rhs->offset;
            }
        };

        std::set<Block*, Cmp> blocks_by_offset;

        // Gather the free blocks
        for(auto& block: blocks_) {
            if(block.used == 0) {
                blocks_by_offset.insert(const_cast<Block*>(&block));
            }
        }

        Block* prev = nullptr;
        for(auto block: blocks_by_offset) {
            if(prev && (prev->offset + prev->size) == block->offset) {
                prev = update_size(prev, block->size);
                block = update_size(block, -int(block->size));
            } else {
                prev = block;
            }
        }

        remove_zero_sized_free_blocks();
    }

    void remove_zero_sized_free_blocks() {
        for(auto it = blocks_.begin(); it != blocks_.end();) {
            if(it->size == 0) {
                it = blocks_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

} // namespace smlt
