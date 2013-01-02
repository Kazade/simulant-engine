#ifndef MANAGED_H
#define MANAGED_H

#include <stdexcept>
#include <tr1/memory>

class InstanceInitializationError :
    public std::runtime_error {

public:
    InstanceInitializationError():
        std::runtime_error("Couldn't initialize the instance") {}
};

template<typename T>
class Managed {
public:
    typedef std::tr1::shared_ptr<T> ptr;
    typedef std::tr1::weak_ptr<T> wptr;

    static typename Managed<T>::ptr create() {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T());
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U>
    static typename Managed<T>::ptr create(U& p1) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U>
    static typename Managed<T>::ptr create(const U& p1) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V>
    static typename Managed<T>::ptr create(U& p1, V& p2) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V>
    static typename Managed<T>::ptr create(U& p1, const V& p2) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V>
    static typename Managed<T>::ptr create(const U& p1, const V& p2) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W>
    static typename Managed<T>::ptr create(U& p1, V& p2, W& p3) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W>
    static typename Managed<T>::ptr create(const U& p1, V& p2, W& p3) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W>
    static typename Managed<T>::ptr create(const U& p1, const V& p2, const W& p3) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W>
    static typename Managed<T>::ptr create(U& p1, const V& p2, const W& p3) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W, typename X>
    static typename Managed<T>::ptr create(U& p1, V& p2, W& p3, X& p4) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3, p4));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    template<typename U, typename V, typename W, typename X>
    static typename Managed<T>::ptr create(const U& p1, const V& p2, const W& p3, const X& p4) {
        typename Managed<T>::ptr instance = typename Managed<T>::ptr(new T(p1, p2, p3, p4));
        if(!instance->init()) {
            throw InstanceInitializationError();
        }
        return instance;
    }

    virtual ~Managed() {}
    virtual bool init() { return true; }

protected:
    Managed() {}

};

#endif // MANAGED_H
