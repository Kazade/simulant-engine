#ifndef AUTO_WEAKPTR_H
#define AUTO_WEAKPTR_H

#include <memory>

class ObjectDestroyedError : public std::runtime_error {
public:
    ObjectDestroyedError(const std::string& what):
        std::runtime_error(what) {}
};

template<typename T>
class AutoWeakPtr {
public:
    AutoWeakPtr(std::weak_ptr<T> object):
        weak_ptr_(object) {}

    template<typename U>
    AutoWeakPtr(const AutoWeakPtr<U>& other):
        weak_ptr_(other.weak_ptr_) {
    }

    std::shared_ptr<T> operator->() {
        if(weak_ptr_.expired()) {
            throw ObjectDestroyedError("Unable to get a lock on the object");
        }

        return weak_ptr_.lock();
    }
    const std::shared_ptr<T> operator->() const {
        if(weak_ptr_.expired()) {
            throw ObjectDestroyedError("Unable to get a lock on the object");
        }

        return weak_ptr_.lock();
    }

    explicit operator bool() const {
        return !weak_ptr_.expired();
    }

private:
    std::weak_ptr<T> weak_ptr_;
};

#endif // AUTO_WEAKPTR_H
