#pragma once

/* FIXME: Only works if a subclass of Managed<T> */

#include <utility>
#include <vector>
#include <array>
#include <unordered_set>
#include <memory>

#include "../threads/atomic.h"
#include "managed.h"
#include "vector_pool.h"

namespace smlt {

namespace _manual_manager_impl {
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

#ifdef _arch_dreamcast
    // Dreamcast is far more memory constrained
    // so we allocate in smaller chunks so that
    // memory isn't wasted
    const static std::size_t chunk_size = 32;
#else
    const static std::size_t chunk_size = 128;
#endif

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
            container_(container),
            change_counter_(container->change_counter_) {

            assert(container_);

            // If we've asked for the end, or the container is empty
            // we return 1 past the final
            if(pos == POSITION_END || !container_->_size()) {
                current_ = container_->_capacity();
            } else {
                while(!container_->_contains(id_type(current_ + 1))) {
                    current_++;
                }
            }
        }

        iterator& operator++() {
            /* Check the parent container didn't change */
            assert(change_counter_ == container_->change_counter_);

            while(1) {
                current_++;

                if(current_ >= container_->_capacity()) {
                    break;
                }

                if(container_->_contains(id_type(current_ + 1))) {
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
            assert(change_counter_ == container_->change_counter_);

            id_type id(current_ + 1);
            auto ret = container_->_get(id);
            assert(ret);
            return ret;
        }

    private:
        ManualManager* container_ = nullptr;
        uint32_t current_ = 0;

#ifndef NDEBUG
        int change_counter_;
#endif
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

    template<typename CB, typename Data>
    void safe_each(const CB& callback, Data* data=nullptr) {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        for(auto item: _each()) {
            callback(item, data);
        }
    }

    /* Un-safe version */
    std::size_t _size() const {
        return pool_.size();
    }

    // Returns the number of items
    std::size_t size() const {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        return _size();
    }

    /* Un-safe version */
    std::size_t _capacity() const {
        return pool_.capacity();
    }

    // The current capacity of the pool
    std::size_t capacity() const {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        return _capacity();
    }

    std::size_t capacity_available() const {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        return pool_.capacity() - pool_.size();
    }

    // Fetch an element by ID, returns nullptr
    // if it doesn't exist
    T* get(id_type id) const {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        return _get(id);
    }

    /* Un-safe version */
    T* _get(id_type id) const {
        if(!_contains(id)) {
            return nullptr;
        }

        return pool_.get(id);
    }

    // Makes a new object as a subclass
    template<typename Derived, typename... Args>
    Derived* make_as(Args&&... args) {
#ifndef NDEBUG
        change_counter_++;
#endif

        static_assert(_manual_manager_impl::contains<Derived, T, Subtypes...>::value, "Requested unlisted type");
        static_assert(sizeof(Derived) <= PoolType::max_element_size(), "Something went wrong with size calculation");

        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);

        auto p = pool_.template alloc<Derived, Args...>(std::forward<Args>(args)...);
        Derived* ret = dynamic_cast<Derived*>(p.second);
        ret->_bind_id_pointer(ret);
        return ret;
    }

    // Makes a new object, resizing the pool if necessary
    template<typename... Args>
    T* make(Args&&... args) {
#ifndef NDEBUG
        change_counter_++;
#endif

        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);

        auto alloc = &PoolType::template alloc<T, Args...>;
        auto ret = (pool_.*alloc)(std::forward<Args>(args)...).second;
        ret->_bind_id_pointer(ret);
        return ret;
    }

    // Mark the element for destruction at clean_up
    // returns true if the object wasn't already marked
    // to be destroyed
    bool destroy(id_type id) {
        thread::Lock<thread::Mutex> guard(release_mutex_);
        if(to_release_.count(id)) {
            return false;
        }

        to_release_.insert(id);
        return true;
    }

    // This deletes the object immediately, without any
    // grace period. Returns true if the object wasn't already
    // queued to be destroyed
    bool destroy_immediately(id_type id) {
#ifndef NDEBUG
        change_counter_++;
#endif

        thread::Lock<thread::RecursiveMutex> guard2(pool_mutex_);
        {
            thread::Lock<thread::Mutex> guard1(release_mutex_);
            if(to_release_.count(id)) {
                return false;
            }
            to_release_.erase(id);
        }

        pool_.release(id);
        return true;
    }

    // Clean up deleted objects
    void clean_up() {
#ifndef NDEBUG
        change_counter_++;
#endif
        thread::Lock<thread::RecursiveMutex> guard2(pool_mutex_);

        release_mutex_.lock();
        while(to_release_.size()) {
            auto id = *to_release_.begin();
            to_release_.erase(id);

            /* pool_.release might try to get the release
             * mutex indirectly */
            release_mutex_.unlock();
                pool_.release(id);
            release_mutex_.lock();
        }
        release_mutex_.unlock();
    }

    void destroy_all() {
        for(auto thing: _each()) {
            assert(thing);
            destroy(thing->id());
        }
    }

    // Immediately clear the manager
    void clear() {
#ifndef NDEBUG
        change_counter_++;
#endif

        thread::Lock<thread::RecursiveMutex> guard1(pool_mutex_);
        {
            thread::Lock<thread::Mutex> guard2(release_mutex_);
            to_release_.clear();
        }

        pool_.clear();
    }

    /* Un-safe version */
    bool _contains(id_type id) const {
        return pool_.used(id);
    }

    // Returns true if the ID is allocated
    bool contains(id_type id) const {
        thread::Lock<thread::RecursiveMutex> guard(pool_mutex_);
        return _contains(id);
    }

    T* clone(id_type id, this_type* target_manager=nullptr) {
        target_manager = target_manager || this;
        auto source = get(id);
        auto copy = target_manager->make(*source); // Copy construction
        return copy;
    }

    bool is_marked_for_destruction(id_type id) const {
        thread::Lock<thread::Mutex> guard(release_mutex_);
        return to_release_.count(id) > 0;
    }

private:
    typedef VectorPool<T, id_type, chunk_size, Subtypes...> PoolType;

    mutable thread::RecursiveMutex pool_mutex_;
    PoolType pool_;

    mutable thread::Mutex release_mutex_;
    std::unordered_set<id_type> to_release_;

#ifndef NDEBUG
    /* Used to detect threading issues */
    thread::Atomic<int> change_counter_;
#endif
};


}
