#pragma once

/* Polylist is a container with the following properties:
 *
 *  - It can store any leaf element from a heirarchy that subclass
 *    Base
 *  - It pools memory into chunked allocations of ChunkSize
 *  - It provides an id on insertion that allows for constant-time
 *    lookups.
 *
 * Usage:
 *
 * Polylist<BaseThing, Thing> list;
 *
 * auto pair = list.create("mything");
 * auto it = pair.first;
 * auto id = pair.second;
 *
 * (*it) == list[id]; // true
 * list[some_other_id] == nullptr; // true
 *
 * list.erase(it); // or list.erase(id);
 */

#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>

namespace smlt {

namespace _polylist {

struct EntryMeta {
    EntryMeta* prev = nullptr;
    EntryMeta* next = nullptr;
    void* entry = nullptr;
    bool is_free = true;

    std::size_t chunk = 0;  // Chunk index
    std::size_t index = 0;  // Index in the chunk
};

template<typename... Others> struct MaxSize {
  static constexpr std::size_t value = 0;
};

template<typename A, typename... Others> struct MaxSize<A, Others...> {
    static constexpr std::size_t a_size = sizeof(A);
    static constexpr std::size_t b_size = MaxSize<Others...>::value;
    static constexpr std::size_t value = (a_size > b_size) ? a_size : b_size;
};

}

template<typename Base, typename... Classes>
class Polylist {
public:
    const static int entry_size = _polylist::MaxSize<Base, Classes...>::value;
    const std::size_t chunk_size;

    template<bool IsConst>
    class PolylistIterator {
    private:
        PolylistIterator(const Polylist* owner, _polylist::EntryMeta* meta):
            owner_(owner),
            current_(meta) {

        }

        const Polylist* owner_;
        _polylist::EntryMeta* current_;

        friend class Polylist;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Base*;
        using difference_type = uint32_t;
        using pointer = Base**;
        using reference = Base*&;

        bool operator==(const PolylistIterator& rhs) const {
            return current_ == rhs.current_;
        }

        bool operator!=(const PolylistIterator& rhs) const {
            return current_ != rhs.current_;
        }

        PolylistIterator& operator++() {
            if(current_->next) {
                current_ = current_->next;
            } else if(owner_->chunks_.size() > current_->chunk + 1) {
                /* If next is null, we're at the end of the current chunk
                 * so move to the next one */
                current_ = owner_->chunks_[current_->chunk + 1].used_list_head_;
            } else {
                /* We're done */
                current_ = nullptr;
            }

            return *this;
        }

        reference operator*() const {
            return (reference) current_->entry;
        }

        pointer operator->() const {
            if(!current_) {
                return nullptr;
            }

            return (pointer) &current_->entry;
        }
    };

    friend class PolylistIterator<true>;
    friend class PolylistIterator<false>;

    typedef PolylistIterator<false> iterator;
    typedef PolylistIterator<true> const_iterator;
    typedef std::size_t id;

    Polylist(std::size_t chunk_size):
        chunk_size(chunk_size) {}

    /* Create a new element using placement new, with the set of
     * args for the constructor. */
    template<typename T, typename... Args>
    std::pair<T*, id> create(Args&& ...args) {
        id new_id;
        auto meta = alloc_entry(&new_id);

        /* Construct the object */
        T* ret = new (meta->entry) T(std::forward<Args>(args)...);

        return std::make_pair(ret, new_id);
    }

    iterator find(const id& i) const {
        auto obj = (*this)[i];
        if(obj) {
            _polylist::EntryMeta* meta = (_polylist::EntryMeta*) (((byte*) obj) + entry_size);
            assert(!meta->is_free);

            return iterator(
                this, meta
            );
        } else {
            return end();
        }
    }

    /* Erase an element, and return an iterator to the next used
     * element */
    iterator erase(iterator it) {
        auto meta = it.current_;

        assert(meta);

        auto next = iterator(this, meta->next);

        assert(!meta->is_free);

        /* Remove from the used list */
        if(meta->prev) meta->prev->next = meta->next;
        if(meta->next) meta->next->prev = meta->prev;

        auto& chunk = chunks_[meta->chunk];
        if(chunk.used_list_head_ == meta) chunk.used_list_head_ = meta->next;
        if(chunk.used_list_tail_ == meta) chunk.used_list_tail_ = meta->prev;

        /* Add to the free list */
        meta->is_free = true;
        meta->prev = chunk.free_list_tail_;
        meta->next = nullptr;

        chunk.free_list_tail_ = meta;
        if(!chunk.free_list_head_) {
            chunk.free_list_head_ = meta;
        }

        assert(size_ > 0);
        size_--;

        /* Call destructor (if stuff goes down then at least the lists are consistent) */
        Base* obj = (Base*) meta->entry;
        obj->~Base();

        return next;
    }

    void reserve(std::size_t amount) {
        while(capacity() < amount) {
            push_chunk();
        }
    }

    iterator begin() const {
        for(auto& chunk: chunks_) {
            if(chunk.used_list_head_){
                return iterator(this, chunk.used_list_head_);
            }
        }

        return iterator(this, nullptr);
    }

    iterator end() const {
        return iterator(this, nullptr);
    }

    /* Destroy all the elements */
    void clear() {
        for(auto it = begin(); it != end();) {
            it = erase(it);
        }
    }

    Base* operator[](id i) {
        return object_from_id(i);
    }

    const Base* operator[](id i) const {
        return object_from_id(i);
    }

    std::size_t size() const {
        return size_;
    }

    std::size_t capacity() const {
        return chunk_size * chunks_.size();
    }

private:
    typedef uint8_t byte;
    std::size_t size_ = 0;

    struct EntryWithMeta {
        byte entry[entry_size] __attribute__((aligned(8)));
        _polylist::EntryMeta meta;
    };

    struct Chunk {
        byte* data = nullptr;

        _polylist::EntryMeta* free_list_head_ = nullptr;
        _polylist::EntryMeta* free_list_tail_ = nullptr;

        _polylist::EntryMeta* used_list_head_ = nullptr;
        _polylist::EntryMeta* used_list_tail_ = nullptr;
    };


    /* Each chunk is a dynamically allocated block of data that is
     * sizeof(ElementWithMeta) * ChunkSize in size */
    std::vector<Chunk> chunks_;

    void push_chunk() {
        /* Allocate the new chunk */
        Chunk new_chunk;
        new_chunk.data = new byte[sizeof(EntryWithMeta) * chunk_size];

        /* Get pointers to the first meta (which follows the actual entry)
         * and the previous in the list, which would be the current free tail
         */
        EntryWithMeta* ewm = (EntryWithMeta*) new_chunk.data;


        /* Set the free list head to the first thing */
        new_chunk.free_list_head_ = &ewm->meta;

        /* Go through all metas and set prev/next pointers */
        for(std::size_t i = 0; i < chunk_size; ++i) {
            _polylist::EntryMeta* meta = &ewm->meta;
            _polylist::EntryMeta* prev = (i == 0) ? nullptr : &(ewm - 1)->meta;

            meta->prev = prev;
            meta->is_free = true;

            meta->chunk = chunks_.size();
            meta->index = i;  // Set the ID
            meta->entry = ewm->entry;

            if(i == chunk_size - 1) {
                meta->next = nullptr;
                new_chunk.free_list_tail_ = meta;
            } else {
                meta->next = &(ewm + 1)->meta;
                ewm++;
            }
        }

        /* Finally, make the new chunk available and set the free list tail
         * to the last of the new chunk */
        chunks_.push_back(new_chunk);
    }

    bool pop_chunk() {
        if(chunks_.empty()) {
            return false;
        }

        auto& chunk = chunks_.back();

        auto it = iterator(this, chunk.used_list_head_);

        /* End really means end, because this is the final chunk *
         * this wouldn't work to delete any chunk as the end iterator
         * would be the used head of the next chunk in that case */
        auto end = iterator(this, nullptr);

        for(; it != end;) {
            it = erase(it);
        }

        delete [] chunk.data;
        chunk.data = nullptr;

        chunks_.pop_back();

        return true;
    }

    _polylist::EntryMeta* alloc_entry(id* new_id) {
        uint32_t i = 0;
        for(auto& chunk: chunks_) {
            if(chunk.free_list_head_) {
                _polylist::EntryMeta* result = chunk.free_list_head_;

                chunk.free_list_head_ = chunk.free_list_head_->next;
                if(chunk.free_list_tail_ == result) {
                    chunk.free_list_tail_ = result->prev;
                }

                /* Add to the end of the used list */
                result->prev = chunk.used_list_tail_;
                if(result->prev) result->prev->next = result;
                chunk.used_list_tail_ = result;

                /* Make the head point to this if this is the first one */
                if(!chunk.used_list_head_) {
                    chunk.used_list_head_ = result;
                }

                result->next = nullptr;
                result->is_free = false;

                *new_id = ((i * chunk_size) + result->index) + 1;

                ++size_;

                return result;
            }

            ++i;
        }

        /* If no free entry was found, we need a new chunk */
        push_chunk();

        return alloc_entry(new_id);
    }

    Base* object_from_id(id i) const {
        uint32_t idx = i - 1;
        std::size_t chunk_id = (idx / chunk_size);
        std::size_t index = (idx % chunk_size);

        EntryWithMeta* ewm = reinterpret_cast<EntryWithMeta*>(
            &chunks_[chunk_id].data[index * sizeof(EntryWithMeta)]
        );

        return (ewm->meta.is_free) ?
            nullptr : reinterpret_cast<Base*>(ewm->entry);
    }
};

}
