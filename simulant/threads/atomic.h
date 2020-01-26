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
    Atomic(T v):
        v_(v) {}

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
