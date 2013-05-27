#ifndef MANAGER_BASE_H
#define MANAGER_BASE_H

#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <thread>
#include <mutex>

#include <sigc++/sigc++.h>

namespace kglt {
namespace generic  {

class NoSuchObjectError : public std::logic_error {
public:
    NoSuchObjectError(const std::string& type_name):
        std::logic_error("The manager does not contain an object with the specified ID: " + type_name) {}
};

class BaseManager {
public:
    virtual ~BaseManager() {}

protected:
    mutable std::recursive_mutex manager_lock_;
};

template<typename T>
class IncrementalGetNextID {
public:
    T operator()() {
        static uint32_t counter = 0;
        return T(++counter);
    }
};

}
}

#endif // MANAGER_BASE_H
