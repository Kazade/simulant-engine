#pragma once

#include <cstddef>

namespace smlt {

template<typename T>
class default_init_ptr {
    /* This class exists to protect against forgetting to initialize a
     * an object pointer. It could theoretically cause a performance issue
     * but no more so than any other smart pointer, and hopefully the compiler
     * will be smart enough to optimise any cost away.
     *
     * If this does cause a performance hit on a particular platform
     * (Dreamcast, I'm looking at you!) then it can always be typedef'd away
     */
private:
    T* ptr_ = nullptr;

public:
    typedef T element_type;

    default_init_ptr(): ptr_(nullptr) {}
    default_init_ptr(std::nullptr_t): ptr_(nullptr) {}
    default_init_ptr(const default_init_ptr<T>&) = default;
    default_init_ptr<T>& operator=(const default_init_ptr<T>&) = default;
    default_init_ptr<T>& operator=(std::nullptr_t) {
        ptr_ = nullptr;
        return *this;
    }

    default_init_ptr(T* p):
        ptr_(p) {}

    default_init_ptr<T>& operator=(T* p) {
        ptr_ = p;
        return *this;
    }

    T* operator->() const {
        return ptr_;
    }

    T& operator*() const {
        return *ptr_;
    }

    T& operator[](std::size_t i) const {
        return ptr_[i];
    }

    operator T*() const {
        return ptr_;
    }
};


}
