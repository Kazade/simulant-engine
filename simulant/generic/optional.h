#pragma once
#include <cstddef>
#include <cassert>
#include <utility>
#include <cstdint>

namespace smlt {

/* When all compilers support the true optional class we can delete this. This exists largely for compatibility
 * with old GCC for the Dreamcast port */

template<typename T>
class optional {
public:
    optional() = default;

    template<typename U>
    optional(optional<U>&& other) {
        auto src = other.value_ptr();
        if(src) {
            set_value(std::move(*src));
        } else {
            reset();
        }
    }

    optional(const T& value) {
        set_value(value);
    }

    optional(T&& value) {
        set_value(std::move(value));
    }

    optional(const optional<T>& rhs) {
        if(rhs.has_value()) {
            set_value(rhs.value());
        }
    }

    optional& operator=(const optional& other) {
        if(other.has_value()) {
            set_value(other.value());
        }

        return *this;
    }

    explicit operator bool() const {
        return has_value();
    }

    bool has_value() const { return has_value_; }

    const T& value() const {
        assert(has_value());
        return *value_ptr();
    }

    T& value() {
        assert(has_value());
        return *value_ptr();
    }

    T value_or(T&& def) const {
        bool(*this) ? **this : static_cast<T>(std::forward<T>(def));
    }

    T value_or(T&& def) {
        return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<T>(def));
    }

    const T* operator->() const {
        assert(has_value());
        return value_ptr();
    }

    T* operator->() {
        assert(has_value());
        return value_ptr();
    }

    const T& operator*() const {
        assert(has_value());
        return *value_ptr();
    }

    T& operator*() {
        assert(has_value());
        return *value_ptr();
    }

private:
    uint8_t data_[sizeof(T)] __attribute__((aligned(8)));
    bool has_value_ = false;

    T* value_ptr() const {
        return (has_value()) ? (T*) (data_) : nullptr;
    }

    void set_value(const T& value) {
        if(has_value()) {
            reset();
        }

        new (data_) T(value);
        has_value_ = true;
    }

    void set_value(T&& value) {
        if(has_value()) {
            // We destroy the current value if we have one
            reset();
        }

        new (data_) T(std::move(value));
        has_value_ = true;
    }

    void reset() {
        T* v = value_ptr();
        if(v) {
            has_value_ = false;
            v->~T();
        }
    }
};


}
