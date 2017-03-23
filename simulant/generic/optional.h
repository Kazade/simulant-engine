#pragma once
#include <cassert>

namespace smlt {

/* When all compilers support the true optional class we can delete this. This exists largely for compatibility
 * with old GCC for the Dreamcast port */

template<typename T>
class optional {
public:
    optional():
        has_value_(false) {

    }

    optional(T&& value):
        has_value_(true),
        value_(value) {

    }

    optional(const optional<T>& rhs):
        has_value_(rhs.has_value_),
        value_(std::move(rhs.value_)){

    }

    explicit operator bool() const {
        return has_value();
    }

    bool has_value() const { return has_value_; }

    void reset() {
        has_value_ = false;
        value_ = T();
    }

    T value() const {
        assert(has_value());
        return value_;
    }

    T value() {
        assert(has_value());
        return value_;
    }

    T value_or(T&& def) const {
        bool(*this) ? **this : static_cast<T>(std::forward<T>(def));
    }

    T value_or(T&& def) {
        return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<T>(def));
    }

    const T* operator->() const {
        assert(has_value());
        return &value_;
    }

    T* operator->() {
        assert(has_value());
        return &value_;
    }

    const T& operator*() const {
        assert(has_value());
        return value_;
    }

    T& operator*() {
        assert(has_value());
        return value_;
    }

private:
    bool has_value_ = false;
    T value_;
};


}
