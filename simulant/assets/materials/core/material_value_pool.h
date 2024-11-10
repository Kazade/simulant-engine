#pragma once

#include <cstdint>
#include <memory>
#include <numeric>
#include <set>

namespace smlt {

namespace _mat_pool {

struct BlockHeader {
    MaterialPropertyType type;
    uint16_t refcount = 0;
};

struct FreeBlock {
    std::size_t offset;
    std::size_t size;

    // Free blocks should be ordered by increasing size
    // so that when allocating we find the smallest
    // available first
    bool operator<(const FreeBlock& rhs) const {
        return size < rhs.size;
    }
};

typedef uint16_t block_header;
/*
 * Really basic allocator over a vector that allocates dynamically sized
 * blocks with the specified alignments */
template<int Alignment>
class DynamicAlignedAllocator {
public:
    static constexpr int alignment = Alignment;

    uint8_t* allocate(std::size_t size) {
        // We can't iterate the set while manipulating the set
        // so we iterate this instead
        std::vector<FreeBlock*> free_block_ptrs;
        for(auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            free_block_ptrs.push_back(const_cast<FreeBlock*>(&(*it)));
        }

        for(auto free_block_ptr: free_block_ptrs) {
            auto size_header_size = sizeof(block_header);
            auto target_addr =
                (data_ + free_block_ptr->offset) + size_header_size;
            auto alignment_offset = uintptr_t(target_addr) % alignment;
            auto required_size =
                (alignment - alignment_offset) + size + size_header_size;

            if(free_block_ptr->size < required_size) {
                // Reduce the size of the free block, this might
                // make it empty
                auto addr = data_ + free_block_ptr->offset;
                free_block_ptr = update_size(free_block_ptr, -required_size);
                free_block_ptr->offset += required_size;
                block_header* header = reinterpret_cast<block_header*>(addr);
                *header = required_size;
                return addr;
            }
        }

        // No free blocks? We need to allocate a new block and then update
        auto additional_bytes = round_to_alignment(sizeof(block_header) + size);
        uint8_t* new_data = (uint8_t*)smlt::aligned_alloc(
            alignment, allocated_size_ + additional_bytes);
        auto offset = allocated_size_;
        memcpy(new_data, data_, allocated_size_);
        allocated_size_ = allocated_size_ + additional_bytes;

        if(realloc_callback_) {
            // Notify the callback so we can update pointers
            realloc_callback_(data_, new_data, user_data_);
        }

        free(data_);
        data_ = new_data;
        free_blocks_.insert(FreeBlock{offset, additional_bytes});
        return data_ + offset;
    }

    void deallocate(const uint8_t* ptr) {
        auto header = reinterpret_cast<const block_header*>(ptr);
        FreeBlock new_block{ptr - data_, *header};
        free_blocks_.insert(new_block);
        merge_free_blocks();
    }

    void set_realloc_callback(void (*callback)(uint8_t*, uint8_t*, void*),
                              void* user_data);

    std::size_t capacity() const {
        return allocated_size_;
    }

    std::size_t used() const {
        return capacity() -
               std::accumulate(free_blocks_.begin(), free_blocks_.end(), 0,
                               [](std::size_t n, const FreeBlock& b) {
            return b.size + n;
        });
    }

private:
    std::size_t round_to_alignment(std::size_t size) {
        return ((size + alignment / 2) / alignment) * alignment;
    }

    uint8_t* data_ = nullptr;
    std::size_t allocated_size_ = 0;
    std::set<FreeBlock> free_blocks_;
    void (*realloc_callback_)(uint8_t*, uint8_t*, void*);
    void* user_data_;

    /* You can't update the property of a value in a set that will
     * change the order, so we have to remove, copy + change, reinsert */
    FreeBlock* update_size(const FreeBlock* block, int change) {
        auto it = std::find_if(free_blocks_.begin(), free_blocks_.end(),
                               [block](const FreeBlock& b) {
            return &b == block;
        });

        if(it == free_blocks_.end()) {
            return nullptr;
        }

        auto new_block = *it;
        new_block.size += change;
        free_blocks_.erase(it);
        it = free_blocks_.insert(new_block).first;
        return const_cast<FreeBlock*>(&(*it));
    }

    void merge_free_blocks() {
        // Merge consecutive free blocks by first ordering by
        // offset, and then iterating and moving the size to the
        // previous block, finally go through and remove zero-sized
        // blocks
        struct Cmp {
            bool operator()(const FreeBlock* lhs, const FreeBlock* rhs) const {
                return lhs->offset < rhs->offset;
            }
        };

        std::set<FreeBlock*, Cmp> blocks_by_offset;
        for(auto& block: free_blocks_) {
            blocks_by_offset.insert(const_cast<FreeBlock*>(&block));
        }

        FreeBlock* prev = nullptr;
        for(auto block: blocks_by_offset) {
            if(prev && (prev->offset + prev->size) == block->offset) {
                prev = update_size(prev, block->size);
                block = update_size(block, -block->size);
            } else {
                prev = block;
            }
        }

        remove_zero_sized_free_blocks();
    }

    void remove_zero_sized_free_blocks() {
        for(auto it = free_blocks_.begin(); it != free_blocks_.end();) {
            if(it->size == 0) {
                it = free_blocks_.erase(it);
            } else {
                ++it;
            }
        }
    }
};

} // namespace _mat_pool

/* We need to be able to defrag the material value pool, so we return pointers
   which are ref-counted to tell if values are still used and maintain our own
   copies in the pool so we can update pointers if memory shifts around */
class MaterialPropertyValuePointer {
    MaterialPropertyType type;
    uint8_t* data = nullptr;
    std::shared_ptr<int> refCount = std::make_shared<int>(0);

public:
    MaterialPropertyValuePointer() = default;
    MaterialPropertyValuePointer(const MaterialPropertyValuePointer& other) {
        data = other.data;
        refCount = other.refCount;
        (*refCount)++;
    }

    MaterialPropertyValuePointer&
        operator=(const MaterialPropertyValuePointer& other) {
        if(this == &other) {
            return *this;
        }

        data = other.data;
        refCount = other.refCount;
        (*refCount)++;

        return *this;
    }

    ~MaterialPropertyValuePointer() {
        --(*refCount);
    }

    template<typename T>
    T& get() {
        return *reinterpret_cast<T*>(data);
    }

    template<typename T>
    const T& get() const {
        return *reinterpret_cast<const T*>(data);
    }
};

} // namespace smlt
