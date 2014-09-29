#ifndef MANAGED_H
#define MANAGED_H

#include <stdexcept>
#include <memory>
#include <kazbase/unicode.h>

class InstanceInitializationError :
    public std::runtime_error {

public:
    InstanceInitializationError():
        std::runtime_error("Couldn't initialize the instance") {}

    InstanceInitializationError(const unicode& type):
        std::runtime_error((type + " could not be initialized").encode()) {

    }
};

template<typename T>
void deleter(T* obj) {
    obj->cleanup();
    delete obj;
}

template<typename T>
class Managed {
public:
    typedef std::shared_ptr<T> ptr;
    typedef std::weak_ptr<T> wptr;

    template<typename... Args>
    static typename Managed<T>::ptr create(Args&&... args) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(
            new T(std::forward<Args>(args)...),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }

    static typename Managed<T>::ptr create() {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(
            new T(),
            std::bind(&deleter<T>, std::placeholders::_1)
        );

        if(!instance->init()) {
            throw InstanceInitializationError(typeid(T).name());
        }
        return instance;
    }

    virtual ~Managed() {}
    virtual bool init() { return true; }
    virtual void cleanup() {}

    bool uses_gc() const { return uses_gc_; }
    void enable_gc(bool value=true) { uses_gc_ = value; }

protected:
    template<typename...Args>
    Managed(Args&&... args) {}

private:
    bool uses_gc_ = true;
};

#endif // MANAGED_H
