#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <cstdint>
#include "safe_bool.h"

template<uint32_t T>
class UniqueID : public safe_bool<UniqueID<T> >{
public:
    UniqueID():
        id_(0) {}

    explicit UniqueID(uint32_t id):
        id_(id) {}

    bool operator==(const UniqueID<T>& other) const {
        return this->id_ == other.id_;
    }

    void operator=(const UniqueID<T>& other) {
        this->id_ = other.id_;
    }

    bool operator<(const UniqueID<T>& other) const {
        return this->id_ < other.id_;
    }

    bool operator!=(const UniqueID<T>& other) const {
        return !(*this == other);
    }

public:
    bool boolean_test() const {
        return id_ != 0;
    }

private:
    uint32_t id_;
};

#endif // UNIQUE_ID_H
