#ifndef MANAGER_BASE_H
#define MANAGER_BASE_H

#include <memory>
#include <unordered_map>
#include <stdexcept>
#include <thread>
#include <mutex>

namespace kglt {
namespace generic  {

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
