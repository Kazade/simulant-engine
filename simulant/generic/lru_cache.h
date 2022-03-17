#pragma once

#include <unordered_map>
#include "optional.h"

namespace smlt {

template<typename Key, typename Value>
class LRUCache {
private:
    struct Entry {
        Entry(const Key& key, const Value& value):
            key(key), value(value) {}

        Entry* prev = nullptr;
        Entry* next = nullptr;

        Key key;
        Value value;
    };

    std::unordered_map<Key, Entry*> cache_;
    mutable Entry* entries_ = nullptr;
    mutable Entry* tail_ = nullptr;

    std::size_t max_size_ = 64;

    void pop_tail() {
        if(!tail_) {
            assert(cache_.empty());
            assert(!entries_);
            return;
        }

        Entry* old_tail = tail_;
        tail_ = tail_->prev;
        if(tail_) {
            assert(entries_);
            tail_->next = nullptr;
        } else {
            /* No tail? Must be last entry */
            entries_ = nullptr;
        }

        cache_.erase(old_tail->key);

        delete old_tail;
    }

#ifndef NDEBUG
    void check_valid() const {
        auto it = entries_;
        auto count = 0u;
        while(it) {
            assert(it != it->next);

            ++count;
            it = it->next;
        }

        assert(cache_.size() == count);
    }
#endif

public:
    ~LRUCache() {
        clear();
    }

    optional<Value> get(const Key& key) const {
        auto it = cache_.find(key);
        if(it != cache_.end()) {
            auto entry = it->second;

            if(entry != entries_) {
                // Remove from list
                if(entry->prev) entry->prev->next = entry->next;
                if(entry->next) entry->next->prev = entry->prev;

                if(tail_ == entry) {
                    tail_ = entry->prev;
                }

                // Insert at the beginning
                entry->prev = nullptr;
                entry->next = entries_;
                entries_->prev = entry;
                entries_ = entry;
            }

#ifndef NDEBUG
            check_valid();
#endif
            return optional<Value>(entry->value);
        }

        return optional<Value>();
    }

    void clear() {
        while(size()) {
            pop_tail();
        }
    }

    bool insert(const Key& key, const Value& value) {
        auto it = cache_.find(key);
        if(it != cache_.end()) {
            // Can't insert duplicate key
            return false;
        }

        /* Insert at the beginning */
        Entry* new_entry = new Entry(key, value);

        if(!tail_) {
            /* First, and last */
            tail_ = new_entry;
        }

        new_entry->next = entries_;
        if(entries_) {
            entries_->prev = new_entry;
        }

        entries_ = new_entry;
        cache_.insert(std::make_pair(key, new_entry));

        /* If we grew too big, remove the last entry */
        if(cache_.size() > max_size_) {
            pop_tail();
        }

#ifndef NDEBUG
        check_valid();
#endif
        return true;
    }

    bool erase(const Key& key) {
        auto it = cache_.find(key);
        if(it == cache_.end()) {
            return false;
        }

        assert(it->second.key == key);

        /* Move to the end. Just saves duplicating removal code */
        auto entry = it->second;
        if(entry != tail_) {
            if(entry->prev) entry->prev->next = entry->next;
            if(entry->next) entry->next->prev = entry->prev;

            entry->prev = tail_;
            tail_->next = entry;
            tail_ = entry;
        }

        /* Pop the tail */
        pop_tail();
#ifndef NDEBUG
        check_valid();
#endif
    }

    void set_max_size(std::size_t size) {
        while(cache_.size() > size) {
            pop_tail();
        }

        max_size_ = size;
    }

    std::size_t size() const {
        return cache_.size();
    }

    std::size_t max_size() const {
        return max_size_;
    }
};

}
