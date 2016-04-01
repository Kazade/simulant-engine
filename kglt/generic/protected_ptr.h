#ifndef PROTECTED_PTR_H
#define PROTECTED_PTR_H

#include <memory>
#include <mutex>

class Protectable {
public:
    Protectable() = default;
    virtual ~Protectable() {}

    std::recursive_mutex& mutex() { return mutex_; }

private:
    std::recursive_mutex mutex_;
};

template<typename T>
struct ProtectedPtr {
    typedef std::lock_guard<std::recursive_mutex> guard_type;

    ProtectedPtr() = default;

    ProtectedPtr(std::weak_ptr<T> ref):
        __object(ref.lock()),
        lock_(__object ?
            std::make_shared<guard_type>(__object->mutex()) :
            std::shared_ptr<guard_type>()
        ) {

        static_assert(std::is_base_of<Protectable, T>::value, "Type must subclass Protectable");
    }

    template<typename U>
    ProtectedPtr(const ProtectedPtr<U>& other):
        __object(other.__object),
        lock_(other.lock_) {
    }

    ~ProtectedPtr() {
        __object.reset(); //Release reference
        lock_.reset(); //Unlock
    }

    T* operator->() { return __object.get(); }
    const T* operator->() const { return __object.get(); }
    T* get() const { return __object.get(); }

    explicit operator bool() const {
        return __object.get() != nullptr;
    }

    std::shared_ptr<T> __object;
    std::shared_ptr<guard_type> lock_;
};

#endif // PROTECTED_PTR_H
