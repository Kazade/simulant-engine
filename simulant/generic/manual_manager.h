#pragma once

/* FIXME: Only works if a subclass of Managed<T> */

#include <utility>
#include <vector>
#include <array>
#include <unordered_set>
#include <memory>
#include <mutex>

#include "managed.h"

namespace smlt {

namespace _manual_manager_impl {

    template<typename T, typename IDType, int ChunkSize>
    class VectorPool {
    public:
        typedef T element_type;
        typedef IDType id_type;
        typedef uint8_t slot_id;

        static_assert(ChunkSize < std::numeric_limits<slot_id>::max(), "ChunkSize must be less than 256");

        VectorPool() {
            push_chunk();
        }

        template<typename... Args>
        std::pair<id_type, T*> alloc(Args&&... args) {
            uint32_t i = 0;
            for(auto& chunk: chunks_) {
                auto chunk_id = i++;

                if(chunk->free_slots_.empty()) {
                    continue;
                } else {
                    slot_id slot;
                    {
                        std::lock_guard<std::mutex> g(chunk->lock_);
                        slot = *chunk->free_slots_.begin();
                        chunk->free_slots_.erase(slot);
                    }

                    id_type id = id_for_chunk_slot(chunk_id, slot);

                    uint8_t* new_thing = find_address(id);
                    T* ret = new (new_thing) T(id, args...);

                    ++size_;

                    return std::make_pair(
                        id,
                        ret
                    );
                }
            }

            // No free chunks
            push_chunk();

            return alloc(std::forward<Args>(args)...);
        }

        bool used(id_type id) const {
            auto p = find_chunk_and_slot(id);
            assert(p.first);

            return p.first->free_slots_.count(p.second) == 0;
        }

        T* get(id_type id) const {
            assert(used(id));
            return reinterpret_cast<T*>(find_address(id));
        }

        void release(id_type id) {
            assert(used(id));

            auto p = find_chunk_and_slot(id);
            T* element = get(id);

            // Call the destructor
            element->~T();

            {
                std::lock_guard<std::mutex> g(p.first->lock_);
                // Mark the slot as free
                p.first->free_slots_.insert(p.second);
            }

            --size_;
        }

        std::size_t capacity() const {
            return chunks_.size() * ChunkSize;
        }

        std::size_t size() const {
            return size_;
        }

    private:
        std::mutex chunk_lock_;

        struct Chunk {
            Chunk() {
                for(auto i = 0; i < ChunkSize; ++i) {
                    free_slots_.insert(i);
                }
            }

            static constexpr uint32_t size() {
                return ChunkSize * sizeof(T);
            }

            std::mutex lock_;
            std::unordered_set<slot_id> free_slots_;
            std::array<uint8_t, size()> elements_;
        };

        std::vector<std::shared_ptr<Chunk>> chunks_;

        void push_chunk() {
            std::lock_guard<std::mutex> g(chunk_lock_);
            chunks_.push_back(std::make_shared<Chunk>());
        }

        std::pair<Chunk*, slot_id> find_chunk_and_slot(id_type id) const {
            assert(id.value() > 0);

            // We must subtract one from the id value
            auto v = id.value() - 1;
            auto idx = v / ChunkSize;
            slot_id slot = (v % ChunkSize);

            assert(idx < chunks_.size());
            assert(slot < ChunkSize);

            return std::make_pair(chunks_[idx].get(), slot);
        }

        id_type id_for_chunk_slot(uint32_t chunk_id, slot_id slot) {
            return ((chunk_id * ChunkSize) + slot) + 1;
        }

        uint8_t* find_address(id_type id) const {
            auto chunk = find_chunk_and_slot(id);
            return &chunk.first->elements_.at(chunk.second * sizeof(T));
        }

        uint32_t size_ = 0;
    };
}

template<typename T, typename IDType>
class ManualManager {
public:
    typedef T element_type;
    typedef IDType id_type;
    typedef ManualManager<T, IDType> this_type;
    const static std::size_t chunk_size = 128;

    // Returns the number of items
    std::size_t size() const {
        return pool_.size();
    }

    // The current capacity of the pool
    std::size_t capacity() const {
        return pool_.capacity();
    }

    std::size_t capacity_available() const {
        return pool_.capacity() - pool_.size();
    }

    // Fetch an element by ID, returns nullptr
    // if it doesn't exist
    T* get(id_type id) const {
        if(!contains(id)) {
            return nullptr;
        }

        return pool_.get(id);
    }

    // Makes a new object, resizing the pool if necessary
    template<typename... Args>
    T* make(Args&&... args) {
        T* ret = pool_.alloc(std::forward<Args>(args)...).second;

        if(!ret->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }

        return ret;
    }

    // Makes a new object as a subclass
    template<typename U, typename... Args>
    U* make_as(Args&&... args) {
        return dynamic_cast<U*>(make(std::forward<Args>(args)...));
    }

    // Mark the element for destruction at cleanup
    void destroy(id_type id) {
        to_release_.insert(id);
    }

    // Clean up deleted objects
    void clean_up() {
        while(to_release_.size()) {
            auto id = *to_release_.begin();

            auto elem = get(id);
            if(elem) {
                elem->cleanup();
            }

            pool_.release(id);
            to_release_.erase(id);

        }
    }

    // Immediately clear the manager
    void clear() {

    }

    // Returns true if the ID is allocated
    bool contains(id_type id) const {
        return pool_.used(id);
    }

    T* clone(id_type id, this_type* target_manager=nullptr) {
        target_manager = target_manager || this;
        auto source = get(id);
        auto copy = target_manager->make(*source); // Copy construction
        return copy;
    }

    template<typename Func>
    void each(Func func) {
        /* FIXME: This seems expensive! */
        for(uint32_t i = 0; i < capacity(); ++i) {
            id_type id(i + 1);
            if(contains(id)) {
                func(i, get(id));
            }
        }
    }

    bool is_marked_for_destruction(id_type id) const {
        return to_release_.count(id) > 0;
    }

private:
    _manual_manager_impl::VectorPool<T, id_type, chunk_size> pool_;
    std::unordered_set<id_type> to_release_;
};


}
