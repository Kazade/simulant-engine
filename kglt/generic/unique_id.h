#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <cstdint>
#include <tr1/functional>

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

    UniqueID& operator=(const UniqueID<T>& other) {
        this->id_ = other.id_;
        return *this;
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

    uint32_t value() const { return id_; }
private:
    uint32_t id_;
};

namespace std {
    template<uint32_t T>
    struct hash< UniqueID<T> > {
        size_t operator()(const UniqueID<T>& id) const {
            hash<uint32_t> make_hash;
            return make_hash(id.value());
        }
    };
}


#endif // UNIQUE_ID_H
