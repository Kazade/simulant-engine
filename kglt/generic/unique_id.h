#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <cstdint>

template<uint32_t T>
class UniqueID {
public:
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

private:
    uint32_t id_;
};

#endif // UNIQUE_ID_H
