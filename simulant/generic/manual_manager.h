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

    template<typename T, typename IDType, int ChunkSize, typename ...Subtypes>
    class VectorPool {
    public:
        typedef T element_type;
        typedef IDType id_type;
        typedef uint8_t slot_id;

        static_assert(ChunkSize < std::numeric_limits<slot_id>::max(), "ChunkSize must be less than 256");

        VectorPool() {
            push_chunk();
        }

        ~VectorPool() {
            clear();
        }

        template<typename U, typename... Args>
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

#ifndef NDEBUG
                    // Debugging, set the marker byte
                    uint8_t* marker = new_thing + max_element_size();
                    assert(*marker == 0);

                    *marker = 0xAB;
#endif

                    T* ret = new (new_thing) U(id, args...);

                    if(!ret->init()) {
                        ret->clean_up();
                        ret->~T(); // Call the destructor
                        chunk->free_slots_.insert(slot); // Release the slot
                        throw InstanceInitializationError(typeid(T).name());
                    }

                    ++size_;
#ifndef NDEBUG
                    assert(*marker == 0xAB);
#endif
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
            auto p = find_chunk_and_slot(id);
            if(!p.first) {
                return false;
            }

            return p.first->free_slots_.count(p.second) == 0;
        }

        T* get(id_type id) const {
            assert(used(id));
#ifndef NDEBUG
            uint8_t* addr = find_address(id);
            uint8_t* marker = addr + max_element_size();
            assert(*marker == 0xAB);
            return reinterpret_cast<T*>(addr);
#else
           return reinterpret_cast<T*>(find_address(id));
#endif
        }

        void release(id_type id) {
            assert(used(id));

            auto p = find_chunk_and_slot(id);
            T* element = get(id);

#ifndef NDEBUG
            uint8_t* marker = ((uint8_t*) element) + max_element_size();
            assert(*marker == 0xAB);
            *marker = 0; // Reset the marker
#endif

            if(!element) {
                return;
            }

            element->clean_up();
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

        void clear() {
            while(chunks_.size()) {
                pop_chunk();
            }
        }

        constexpr static uint32_t max_element_size() {
            return MaxSize<T, Subtypes...>::value;
        }

    private:
        std::mutex chunk_lock_;

        template<typename... Others> struct MaxSize {
          static constexpr size_t value = 0;
        };

        template<typename A, typename... Others> struct MaxSize<A, Others...> {
            static constexpr size_t a_size = sizeof(A);
            static constexpr size_t b_size = MaxSize<Others...>::value;
            static constexpr size_t value = (a_size > b_size) ? a_size : b_size;
        };

        struct Chunk {
            Chunk() {
                for(auto i = 0; i < ChunkSize; ++i) {
                    free_slots_.insert(i);
                }
            }

            static constexpr uint32_t size() {
#ifndef NDEBUG
                // If we're debugging, we add marker bytes between each element
                // and initalize them to 0xAB we can then assert that these
                // haven't been overwritten to detect overflows
                return (ChunkSize * MaxSize<T, Subtypes...>::value) + ChunkSize;

#else
                return ChunkSize * MaxSize<T, Subtypes...>::value;
#endif
            }

            std::mutex lock_;
            std::unordered_set<slot_id> free_slots_;
            std::array<uint8_t, size()> elements_ = {}; // Zero-initialize
        };

        std::vector<std::shared_ptr<Chunk>> chunks_;

        void push_chunk() {
            std::lock_guard<std::mutex> g(chunk_lock_);
            chunks_.push_back(std::make_shared<Chunk>());
        }

        void pop_chunk() {
            auto chunk_id = chunks_.size() - 1;
            auto chunk = chunks_.at(chunk_id);
            for(slot_id i = 0; i < ChunkSize; ++i) {
                if(chunk->free_slots_.count(i)) continue;

                id_type id = id_for_chunk_slot(chunk_id, i);
                release(id);
            }

            std::lock_guard<std::mutex> g(chunk_lock_);
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

        uint8_t* find_address(id_type id) const {
            auto chunk = find_chunk_and_slot(id);
#ifndef NDEBUG
            return &chunk.first->elements_.at(chunk.second * (max_element_size() + 1));
#else
            return &chunk.first->elements_.at(chunk.second * max_element_size());
#endif
        }

        uint32_t size_ = 0;
    };

    template < typename Tp, typename... List >
    struct contains : std::true_type {};

    template < typename Tp, typename Head, typename... Rest >
    struct contains<Tp, Head, Rest...>
    : std::conditional< std::is_same<Tp, Head>::value,
        std::true_type,
        contains<Tp, Rest...>
    >::type {};

    template < typename Tp >
    struct contains<Tp> : std::false_type {};
}


template<typename T, typename IDType, typename ...Subtypes>
class ManualManager {
public:
    typedef T element_type;
    typedef IDType id_type;
    typedef ManualManager<T, IDType> this_type;
    const static std::size_t chunk_size = 128;

    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = element_type;
        using difference_type = uint32_t;
        using pointer = element_type**;
        using reference = element_type*;

        enum Position {
            POSITION_BEGIN,
            POSITION_END
        };

        iterator(ManualManager* container, Position pos):
            container_(container) {

            assert(container_);

            // If we've asked for the end, or the container is empty
            // we return 1 past the final
            if(pos == POSITION_END || !container_->size()) {
                current_ = container_->capacity();
            } else {
                while(!container_->contains(id_type(current_ + 1))) {
                    current_++;
                }
            }
        }

        iterator& operator++() {
            while(1) {
                current_++;

                if(current_ == container_->capacity()) {
                    break;
                }

                if(container_->contains(id_type(current_ + 1))) {
                    break;
                }
            }

            return *this;
        }

        iterator operator++(int) {
            auto retval = *this;
            ++(*this);
            return retval;
        }

        bool operator==(const iterator& other) const {
            return container_ == other.container_ && current_ == other.current_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

        reference operator*() const {
            id_type id(current_ + 1);
            auto ret = container_->get(id);
            assert(ret);
            return ret;
        }

    private:
        ManualManager* container_ = nullptr;
        uint32_t current_ = 0;
    };

    class iterator_pair {
    private:
        friend class ManualManager;

        ManualManager* container_ = nullptr;

        iterator_pair(ManualManager* container):
            container_(container) {}
    public:
        iterator begin() {
            return iterator(container_, iterator::POSITION_BEGIN);
        }

        iterator end() {
            return iterator(container_, iterator::POSITION_END);
        }
    };

    virtual ~ManualManager() {
        clear();
    }

    /* Internal: Do not use this explicitly instead
     * use the exposed each_X methods implemented
     * by subclasses + owners */
    iterator_pair _each() {
        return iterator_pair(this);
    }

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
        assert(contains(id));

        if(!contains(id)) {
            return nullptr;
        }

        return pool_.get(id);
    }

    // Makes a new object, resizing the pool if necessary
    template<typename... Args>
    T* make(Args&&... args) {
        return make_as<T, Args...>(std::forward<Args>(args)...);
    }

    // Makes a new object as a subclass
    template<typename Derived, typename... Args>
    Derived* make_as(Args&&... args) {
        static_assert(_manual_manager_impl::contains<Derived, T, Subtypes...>::value, "Requested unlisted type");
        static_assert(sizeof(Derived) <= PoolType::max_element_size(), "Something went wrong with size calculation");

        auto p = pool_.template alloc<Derived, Args...>(std::forward<Args>(args)...);
        Derived* ret = dynamic_cast<Derived*>(p.second);
        ret->_bind_id_pointer(ret);
        return ret;
    }

    // Mark the element for destruction at clean_up
    void destroy(id_type id) {
        to_release_.insert(id);
    }

    // Clean up deleted objects
    void clean_up() {
        while(to_release_.size()) {
            auto id = *to_release_.begin();
            pool_.release(id);
            to_release_.erase(id);

        }
    }

    void destroy_all() {
        for(auto thing: _each()) {
            assert(thing);
            destroy(thing->id());
        }
    }

    // Immediately clear the manager
    void clear() {
        pool_.clear();
        to_release_.clear();
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

    bool is_marked_for_destruction(id_type id) const {
        return to_release_.count(id) > 0;
    }

private:
    typedef _manual_manager_impl::VectorPool<T, id_type, chunk_size, Subtypes...> PoolType;
    PoolType pool_;
    std::unordered_set<id_type> to_release_;
};


}
