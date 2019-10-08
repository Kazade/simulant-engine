#pragma once

#include <mutex>

namespace smlt {

#ifdef _arch_dreamcast
template<typename T>
class atomic {
private:
    mutable std::mutex m_;
    T v_;
public:
    atomic(T v):
        v_(v) {}

    T operator++() noexcept {
        std::lock_guard<std::mutex> g(m_);
        return v_++;
    }

    T operator--() noexcept {
        std::lock_guard<std::mutex> g(m_);
        return v_--;
    }

    T operator++(int) noexcept {
        std::lock_guard<std::mutex> g(m_);
        return ++v_;
    }

    T operator--(int) noexcept {
        std::lock_guard<std::mutex> g(m_);
        return --v_;
    }

    operator T() const {
        std::lock_guard<std::mutex> g(m_);
        return v_;
    }
};
#else

#include <atomic>

template<typename T>
using atomic     =  std::atomic<T>;

#endif
}
