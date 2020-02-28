#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <array>

#include "../threads/thread.h"
#include "../threads/mutex.h"
#include "managed.h"

namespace smlt {

typedef unsigned char byte;

struct MetaBlock {
#ifndef NDEBUG
    // If we're debugging, we add marker bytes between each element
    // and initalize them to 0xAB we can then assert that these
    // haven't been overwritten to detect overflows
    byte marker_byte;
#endif
    // We store the next_used item for fast iteration (e.g. skipping free blocks)
    bool used = false;
    byte* next_used = nullptr;
    byte* prev_used = nullptr;
};

template<typename T, typename IDType, int ChunkSize, typename ...Subtypes>
class VectorPool {
public:
    typedef T element_type;
    typedef IDType id_type;
    typedef byte slot_id;

    template<typename... Others> struct MaxSize {
      static constexpr size_t value = 0;
    };

    template<typename A, typename... Others> struct MaxSize<A, Others...> {
        static constexpr size_t a_size = sizeof(A);
        static constexpr size_t b_size = MaxSize<Others...>::value;
        static constexpr size_t value = (a_size > b_size) ? a_size : b_size;
    };

    const static uint32_t object_size = MaxSize<T, Subtypes...>::value;
    const static uint32_t element_size_unpadded = object_size + sizeof(MetaBlock);
    const static uint32_t element_size = element_size_unpadded + (
        ((element_size_unpadded % 8) != 0) ? (8 - (element_size_unpadded % 8)) : 0
    );
    const static uint32_t array_size = element_size * ChunkSize;

    static_assert(ChunkSize < std::numeric_limits<slot_id>::max(), "ChunkSize must be less than 256");

    VectorPool() {}

    ~VectorPool() {
        clear();
    }

    static MetaBlock* meta_block(const byte* element) {
        if(!element) return nullptr;

        return (MetaBlock*) (element + object_size);
    }

    template<typename U, typename... Args>
    std::pair<id_type, T*> alloc(Args&&... args) {
        uint32_t i = 0;
        for(auto& chunk: chunks_) {
            auto chunk_id = i++;

            if(chunk->used_count_ == ChunkSize) {
                continue;
            } else {
                // We lock the chunk whenever we update metadata
                thread::Lock<thread::Mutex> g(chunk->lock_);

                byte* new_thing = chunk->alloc_free_slot();
                assert(new_thing);

                // Hopefully this works... address of the new element, minus the start
                // should return the number of bytes between the two.
                slot_id slot = (new_thing - &chunk->elements_[0]) / element_size;
                id_type id = id_for_chunk_slot(chunk_id, slot);

#ifndef NDEBUG
                MetaBlock* meta = meta_block(new_thing);
                assert(meta->used); // Should've been allocated
#endif

                /* Construct the object, release the slot
                 * if it throws */
                T* ret = nullptr;
                try {
                    ++chunk->used_count_;
                    ret = new (new_thing) U(id, args...);
                    if(!ret->init()) {
                        ret->clean_up();
                        ret->~T(); // Call the destructor
                        throw InstanceInitializationError(typeid(T).name());
                    }
                } catch(...) {
                    chunk->release_slot(new_thing);
                    --chunk->used_count_;
                    throw;
                }


                ++size_;

                return std::make_pair(
                    id,
                    ret
                );
            }
        }

        // No free chunks
        push_chunk();

        return alloc<U, Args...>(std::forward<Args>(args)...);
    }

    bool used(id_type id) const {
        byte* addr = find_address(id);
        if(!addr) {
            return false;
        }

        return meta_block(addr)->used;
    }

    T* get(id_type id) const {
        assert(used(id));
#ifndef NDEBUG
        byte* addr = find_address(id);
        MetaBlock* meta = meta_block(addr);
        assert(meta->marker_byte == 0xAB);
        return reinterpret_cast<T*>(addr);
#else
        return reinterpret_cast<T*>(find_address(id));
#endif
    }

    void release(id_type id) {
        assert(used(id));

        auto p = find_chunk_and_slot(id);
        T* element = get(id);

        assert(element);
        element->clean_up();
        // Call the destructor
        element->~T();

        thread::Lock<thread::Mutex> g(p.first->lock_);
        p.first->release_slot((uint8_t*) element);

        // Decrement the used count on the chunk
        p.first->used_count_--;
        --size_;
    }

    std::size_t capacity() const {
        return chunks_.size() * ChunkSize;
    }

    std::size_t size() const {
        return size_;
    }

    void clear() {
        /* Release all the slots, but don't delete the chunks */
        std::size_t chunk_id = 0;
        for(auto& chunk: chunks_) {
            while(chunk->first_used_) {

#ifndef NDEBUG
                auto meta = meta_block(chunk->first_used_);
                assert(meta->used);
#endif

                assert(chunk->used_count_);

                slot_id slot = (chunk->first_used_ - &chunk->elements_[0]) / element_size;
                id_type id = id_for_chunk_slot(chunk_id, slot);
                release(id);

                assert(!meta->used);
            }

            ++chunk_id;
        }
    }

    void shrink_to_fit() {
        /* Remove empty chunks from the back */
        while(chunks_.size() && chunks_.back()->used_count_ == 0) {
            pop_chunk();
        }
    }

    constexpr static uint32_t max_element_size() {
        return MaxSize<T, Subtypes...>::value;
    }

private:
    thread::Mutex chunk_lock_;

    struct Chunk {
        Chunk() {
            // Make sure everything is zeroed out to start
            std::fill(elements_, elements_ + array_size, 0);

            // Get a pointer to the first meta block (which follows the object)
            MetaBlock* meta = (MetaBlock*) &elements_[object_size];

            // Initialise all the meta blocks across the whole thing
            for(auto i = 0; i < ChunkSize; ++i) {
#ifndef NDEBUG
                // Set marker bytes (to detect corruption) if in debug mode
                meta->marker_byte = 0;
#endif
                meta->next_used = nullptr;
                meta->prev_used = nullptr;
                meta->used = false;
            }
        }

        void release_slot(byte* addr) {
            auto meta = meta_block(addr);

#ifndef NDEBUG
            assert(meta->marker_byte == 0xAB);
            meta->marker_byte = 0;
#endif

            if(first_used_ == addr) {
                first_used_ = meta->next_used;

                // If we are wiping first used, then the size should be 1 (reducing to zero)
                assert((first_used_) ? used_count_ > 1 : used_count_ == 1);
            }

            if(meta->prev_used) {
                auto prev_meta = meta_block(meta->prev_used);
                prev_meta->next_used = meta->next_used;
            }

            if(meta->next_used) {
                auto next_meta = meta_block(meta->next_used);
                next_meta->prev_used = meta->prev_used;
            }

            meta->used = false;
        }

        byte* alloc_free_slot() {
            if(this->used_count_ == ChunkSize) {
                return nullptr;
            }

            auto insert_before = [](byte* after) -> byte* {
                auto this_addr = after - element_size;
                auto after_meta = meta_block(after);
                auto this_meta = meta_block(this_addr);

                assert(!this_meta->used);
#ifndef NDEBUG
                assert(this_meta->marker_byte == 0);
#endif

                if(after_meta->prev_used) {
                    auto prev_meta = meta_block(after_meta->prev_used);
                    prev_meta->next_used = this_addr;
                }

                this_meta->prev_used = after_meta->prev_used;
                this_meta->next_used = after;
                after_meta->prev_used = this_addr;

                this_meta->used = true;
#ifndef NDEBUG
                this_meta->marker_byte = 0xAB;
#endif
                return this_addr;
            };

            auto insert_after = [](byte* before) -> byte* {
                auto before_meta = meta_block(before);
                auto this_addr = before + element_size;
                auto this_meta = meta_block(this_addr);

                assert(!this_meta->used);
#ifndef NDEBUG
                assert(this_meta->marker_byte == 0);
#endif

                if(before_meta->next_used) {
                    auto next_meta = meta_block(before_meta->next_used);
                    next_meta->prev_used = this_addr;
                }

                this_meta->next_used = before_meta->next_used;
                this_meta->prev_used = before;
                before_meta->next_used = this_addr;

                this_meta->used = true;
#ifndef NDEBUG
                this_meta->marker_byte = 0xAB;
#endif
                return this_addr;
            };

            if(!first_used_) {
                byte* it = &elements_[0];
                auto meta = meta_block(it);

                assert(!meta->used);
#ifndef NDEBUG
                assert(meta->marker_byte == 0);
                meta->marker_byte = 0xAB;
#endif

                meta->used = true;
                meta->next_used = meta->prev_used = nullptr;
                first_used_ = it;
                return it;
            } else {
                byte* it = first_used_;

                while(it) {
                    /* First we look behind this element, if that is unused
                     * then we've jumped over a gap and we can use it */
                    if(it != &elements_[0]) {
                        auto to_alloc = it - element_size;
                        auto to_alloc_meta = meta_block(to_alloc);

                        if(!to_alloc_meta->used) {
                            auto ret = insert_before(it);
                            if(first_used_ == it) {
                                first_used_ = ret;
                            }
                            return ret;
                        }
                    }

                    auto next = meta_block(it)->next_used;

                    if(!next) {
                        /* We've reached the final used block, but, is there
                         * space afterwards? */
                        next = it + element_size;
                        if(next < &elements_[array_size]) {
                            return insert_after(it);
                        } else {
                            // No space left!
                            assert(0 && "This shouldn't happen, should be caught by used_count check at the top of this function");
                            return nullptr;
                        }
                    }

                    it = next;
                }

                return nullptr;
            }
        }

        /* Must be first for alignment reasons */
        byte elements_[array_size];

        thread::Mutex lock_;
        byte* first_used_ = nullptr;
        byte used_count_ = 0;
    };

    std::vector<std::shared_ptr<Chunk>> chunks_;

    void push_chunk() {
        thread::Lock<thread::Mutex> g(chunk_lock_);
        chunks_.push_back(std::make_shared<Chunk>());
    }

    void pop_chunk() {
        auto chunk_id = chunks_.size() - 1;
        auto chunk = chunks_.at(chunk_id);

        auto first_used = chunk->first_used_;
        while(first_used) {
            auto meta = meta_block(first_used);
            assert(meta->used);

            slot_id slot = (first_used - &chunk->elements_[0]) / element_size;
            id_type id = id_for_chunk_slot(chunk_id, slot);
            release(id);

            assert(!meta->used);
            first_used = meta->next_used;
        }

        thread::Lock<thread::Mutex> g(chunk_lock_);
        chunks_.pop_back();
    }

    std::pair<Chunk*, slot_id> find_chunk_and_slot(id_type id) const {
        assert(id.value() > 0);

        // We must subtract one from the id value
        auto v = id.value() - 1;
        auto idx = v / ChunkSize;
        slot_id slot = (v % ChunkSize);

        if(idx >= chunks_.size() || slot >= ChunkSize) {
            return std::make_pair(nullptr, 0);
        }

        return std::make_pair(chunks_[idx].get(), slot);
    }

    id_type id_for_chunk_slot(uint32_t chunk_id, slot_id slot) {
        return ((chunk_id * ChunkSize) + slot) + 1;
    }

    byte* find_address(id_type id) const {
        auto chunk = find_chunk_and_slot(id);
        if(!chunk.first) {
            return nullptr;
        }

        return &chunk.first->elements_[chunk.second * element_size];
    }

    uint32_t size_ = 0;
};



}
