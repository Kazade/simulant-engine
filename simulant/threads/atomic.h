#pragma once

#include "mutex.h"

namespace smlt{
namespace thread {

template<typename T>
class Atomic {
private:
    mutable thread::Mutex m_;
    T v_;
public:
    Atomic():
        v_{0} {}

    constexpr Atomic(T v) noexcept:
        v_(v) {}

    Atomic(const Atomic&) = delete;

    Atomic& operator=(const Atomic&) = delete;
    Atomic& operator=(const Atomic&) volatile = delete;

    T operator=(T desired) noexcept {
        thread::Lock<thread::Mutex> g(m_);
        v_ = desired;
        return v_;
    }

    T operator++() noexcept {
        thread::Lock<thread::Mutex> g(m_);
        return v_++;
    }

    T operator--() noexcept {
        thread::Lock<thread::Mutex> g(m_);
        return v_--;
    }

    T operator++(int) noexcept {
        thread::Lock<thread::Mutex> g(m_);
        return ++v_;
    }

    T operator--(int) noexcept {
        thread::Lock<thread::Mutex> g(m_);
        return --v_;
    }

    operator T() const {
        thread::Lock<thread::Mutex> g(m_);
        return v_;
    }
};

}
}
