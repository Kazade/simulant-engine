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

#include <type_traits>
#include <cassert>
#include <cstdint>
#include <utility>
#include <vector>
#include <memory>
#include <cstring>

namespace smlt {

namespace _polylist {

constexpr std::size_t max(std::size_t a, std::size_t b) {
    return (a > b) ? a : b;
}

template<typename... Ts> struct MaxSize {
    static constexpr std::size_t value = 0;
};

template<typename T, typename... Ts> struct MaxSize<T, Ts...> {
    static constexpr std::size_t value = max(sizeof(T), MaxSize<Ts...>::value);
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

template<typename Base, typename... Classes>
class Polylist {
    struct EntryMeta;

public:
    const static std::size_t entry_size = _polylist::MaxSize<Base, Classes...>::value;
    static_assert(sizeof(Base) <= entry_size, "entry_size was invalid");

    const std::size_t chunk_size;

    Polylist(const Polylist&) = delete;
    Polylist& operator=(const Polylist&) = delete;

    template<bool IsConst>
    class PolylistIterator {
    private:
        PolylistIterator(const Polylist* owner, EntryMeta* meta):
            owner_(owner),
            current_(meta) {

        }

        const Polylist* owner_;
        EntryMeta* current_;

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
                current_ = owner_->chunks_[current_->chunk + 1]->used_list_head_;
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

            return &((reference) current_->entry);
        }
    };

    friend class PolylistIterator<true>;
    friend class PolylistIterator<false>;

    typedef PolylistIterator<false> iterator;
    typedef PolylistIterator<true> const_iterator;
    typedef std::size_t id;

    Polylist(std::size_t chunk_size):
        chunk_size(chunk_size) {
    }

    ~Polylist() {
        clear();
    }

    /* Create a new element using placement new, with the set of
     * args for the constructor. */
    template<typename T, typename... Args>
    std::pair<T*, id> create(Args&&... args) {
        static_assert(std::is_base_of<Base, T>::value, "Must be a subclass of Base");
        static_assert(_polylist::contains<T, Classes...>::value, "T not in Classes...");
        static_assert(sizeof(T) <= entry_size, "sizeof(T) was greater than entry_size");

        id new_id;
        auto ewm = find_free_entry(&new_id);

        /* Construct the object */
        T* ret = new (ewm->data) T(std::forward<Args>(args)...);

        assert((void*) ret == (void*) ewm->data);
        assert((void*) ret == (void*) ewm);

        alloc_entry(ewm, ret);

        assert((Base*) ret == ewm->meta.entry);

        return std::make_pair(ret, new_id);
    }

    iterator find(const id& i) const {
        auto ewm = object_from_id(i);
        Base* obj = ewm->meta.entry;

        if(obj) {
            EntryMeta* meta = &ewm->meta;
            assert(meta->entry);

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

        auto next = iterator(this, meta);
        ++next;

        assert(meta->entry);

        /* Remove from the used list */
        if(meta->prev) meta->prev->next = meta->next;
        if(meta->next) meta->next->prev = meta->prev;

        auto& chunk = chunks_[meta->chunk];
        if(chunk->used_list_head_ == meta) chunk->used_list_head_ = meta->next;
        if(chunk->used_list_tail_ == meta) chunk->used_list_tail_ = meta->prev;

        /* Destroy and then add to the free list, an exception in
           a destructor will terminate anyway so we don't need to worry
           about leaving stuff in an inconsistent state
        */
        Base* obj = meta->entry;
        obj->~Base();

        /* Wipe out the entry */
        auto ewm = get_ewm(meta);
        memset(ewm->data, 0, entry_size);

        meta->entry = nullptr;
        meta->prev = chunk->free_list_tail_;
        meta->next = nullptr;

        chunk->free_list_tail_ = meta;
        if(!chunk->free_list_head_) {
            chunk->free_list_head_ = meta;
        }

        assert(size_ > 0);
        size_--;

        return next;
    }

    void reserve(std::size_t amount) {
        while(capacity() < amount) {
            push_chunk();
        }
    }

    iterator begin() const {
        for(auto& chunk: chunks_) {
            if(chunk->used_list_head_){
                return iterator(this, chunk->used_list_head_);
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

        while(!chunks_.empty()) {
            pop_chunk();
        }
    }

    Base* operator[](id i) {
        return object_from_id(i)->meta.entry;
    }

    const Base* operator[](id i) const {
        return object_from_id(i)->meta.entry;
    }

    bool empty() const {
        return size_ == 0;
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

    struct EntryMeta {
        EntryMeta* prev = nullptr; //4
        EntryMeta* next = nullptr; //4

        Base* entry = nullptr; // 4
        uint32_t padding = 0; // 4

        std::size_t chunk = 0;  // 4
        std::size_t index = 0;  // 4
    };

    typedef struct {
        byte data[entry_size] __attribute__((aligned(8)));
        EntryMeta meta;
    } __attribute__((aligned(8))) EntryWithMeta;

    struct Chunk {
        EntryWithMeta* data = nullptr;

        EntryMeta* free_list_head_ = nullptr;
        EntryMeta* free_list_tail_ = nullptr;

        EntryMeta* used_list_head_ = nullptr;
        EntryMeta* used_list_tail_ = nullptr;
    };

    const static std::size_t slot_size = entry_size + sizeof(EntryMeta);

    /* Each chunk is a dynamically allocated block of data that is
     * sizeof(ElementWithMeta) * ChunkSize in size */
    std::vector<std::shared_ptr<Chunk>> chunks_;

    void push_chunk() {
        /* Allocate the new chunk */
        auto new_chunk = std::make_shared<Chunk>();
        new_chunk->data = new EntryWithMeta[chunk_size];

        /* Get pointers to the first meta (which follows the actual entry)
         * and the previous in the list, which would be the current free tail
         */
        EntryWithMeta* ewm = new_chunk->data;

        /* Set the free list head to the first thing */
        new_chunk->free_list_head_ = &ewm->meta;

        /* Go through all metas and set prev/next pointers */
        for(std::size_t i = 0; i < chunk_size; ++i) {
            EntryMeta* meta = &ewm->meta;
            EntryMeta* prev = (i == 0) ? nullptr : &(ewm - 1)->meta;

            memset(ewm->data, 0, entry_size);

            meta->prev = prev;
            meta->chunk = chunks_.size();
            meta->index = i;  // Set the ID
            meta->entry = nullptr;

            if(i == chunk_size - 1) {
                meta->next = nullptr;
                new_chunk->free_list_tail_ = meta;
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

        auto chunk = chunks_.back();

        auto it = iterator(this, chunk->used_list_head_);

        /* End really means end, because this is the final chunk *
         * this wouldn't work to delete any chunk as the end iterator
         * would be the used head of the next chunk in that case */
        auto end = iterator(this, nullptr);

        for(; it != end;) {
            it = erase(it);
        }

        delete [] chunk->data;
        chunk->data = nullptr;

        chunks_.pop_back();

        return true;
    }

    void alloc_entry(EntryWithMeta* ewm, Base* obj) {
        auto& chunk = chunks_[ewm->meta.chunk];

        if(chunk->free_list_head_ == &ewm->meta) {
            chunk->free_list_head_ = chunk->free_list_head_->next;
        }
        if(chunk->free_list_tail_ == &ewm->meta) {
            chunk->free_list_tail_ = chunk->free_list_tail_->prev;
        }

        /* Add to the end of the used list */
        ewm->meta.prev = chunk->used_list_tail_;
        if(ewm->meta.prev) ewm->meta.prev->next = &ewm->meta;
        chunk->used_list_tail_ = &ewm->meta;

        /* Make the head point to this if this is the first one */
        if(!chunk->used_list_head_) {
            chunk->used_list_head_ = &ewm->meta;
        }

        ewm->meta.next = nullptr;
        ewm->meta.entry = obj;

        ++size_;
    }

    EntryWithMeta* find_free_entry(id* new_id) {
        uint32_t i = 0;
        for(auto chunk: chunks_) {
            if(chunk->free_list_head_) {
                EntryMeta* result = chunk->free_list_head_;

                assert(result->chunk == i);
                *new_id = ((i * chunk_size) + result->index) + 1;

                return get_ewm(result);
            }

            ++i;
        }

        /* If no free entry was found, we need a new chunk */
        push_chunk();

        return find_free_entry(new_id);
    }

    EntryWithMeta* object_from_id(id i) const {
        std::size_t idx = i - 1;
        std::size_t chunk_id = (idx / chunk_size);
        std::size_t index = (idx % chunk_size);

        if(chunk_id >= chunks_.size()) {
            return nullptr;
        }

        if(index >= chunk_size) {
            return nullptr;
        }

        EntryWithMeta* ewm = &chunks_[chunk_id]->data[index];

        assert(chunk_id == ewm->meta.chunk);
        assert(index == ewm->meta.index);

        return ewm;
    }

    EntryWithMeta* get_ewm(EntryMeta* meta) {
        return &chunks_[meta->chunk]->data[meta->index];
    }
};

}
