#pragma once

#include <cstdint>

namespace smlt {

template<typename T> //< Simply for namespacing IDs
class HasAutoID {
private:
    static uint64_t counter;

    uint64_t auto_id_;

public:
    HasAutoID():
        auto_id_(++counter) {
    }

    virtual ~HasAutoID() {}

    uint64_t auto_id() const { return auto_id_; }
};

template<typename T>
uint64_t HasAutoID<T>::counter = 0;


}
