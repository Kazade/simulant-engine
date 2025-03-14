#pragma once

#include <cstddef>
#include <cstring>

namespace smlt {

/**
 * A size-limited vector that is statically allocated
 * (basically an array with a variable length)
 */
template<typename T, std::size_t N>
class LimitedVector {
public:
    LimitedVector() = default;
    LimitedVector(const LimitedVector&) = default;
    LimitedVector& operator=(const LimitedVector&) = default;

    bool operator==(const LimitedVector& rhs) const {
        return std::memcmp(&data_[0], &rhs.data_[0], sizeof(T) * N) == 0;
    }

    bool operator!=(const LimitedVector& rhs) const {
        return !(*this == rhs);
    }

    T& operator[](std::size_t i) {
        assert(i < size());
        return data_[i];
    }

    const T& operator[](std::size_t i) const {
        assert(i < size());
        return data_[i];
    }

    T* data() {
        return data_;
    }

    std::size_t size() const {
        return size_;
    }

    bool empty() const {
        return !size_;
    }

    void clear() {
        while(size_ > 0) {
            data_[size_] = T();
            --size_;
        }
    }

    std::size_t capacity() const {
        return N;
    }

    bool push_back(const T& value) {
        if(size_ == N) {
            return false;
        }

        data_[size_++] = value;
        return true;
    }

    bool push_back(T&& value) {
        if(size_ == N) {
            return false;
        }

        data_[size_++] = std::move(value);
        return true;
    }

    void pop_back() {
        data_[size_] = T();
        --size_;
    }

    struct iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = T;
        using pointer           = T*;
        using reference         = T&;

        reference operator*() const { return *ptr_; }
        pointer operator->() { return ptr_; }

        iterator& operator++() { ptr_++; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++(*this); return tmp; }

        bool operator== (const iterator& b) { return ptr_ == b.ptr_; }
        bool operator!= (const iterator& b) { return ptr_ != b.ptr_; }

    private:
        friend class LimitedVector;

        iterator(T* ptr): ptr_(ptr) {}

        T* ptr_;
    };

    iterator begin() {
        return iterator(data_);
    }

    iterator end() {
        return iterator(data_ + size_);
    }

private:
    T data_[N];
    std::size_t size_ = 0;
};

}
