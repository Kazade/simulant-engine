#pragma once

#include <cstdint>
#include <utility>

namespace smlt {

/*
 * heap_array<T> is a dynamically allocated array, nothing more, nothing less.
 * It just ensures that `delete []` is called correctly.
 */

template<typename T>
class heap_array {
public:
    heap_array(const std::size_t size):
        array_(new T[size]),
        size_(size) {
    }

    ~heap_array() {
        delete [] array_;
    }

    std::size_t size() const {
        return size_;
    }

    T& operator[](std::size_t idx) {
        return array_[idx];
    }

    const T& operator[](std::size_t idx) const {
        return array_[idx];
    }

private:
    T* array_ = nullptr;
    const std::size_t size_;
};

}
