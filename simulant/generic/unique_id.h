#ifndef UNIQUE_ID_H
#define UNIQUE_ID_H

#include <cstdint>
#include <functional>

#include "safe_bool.h"


template<typename ResourceTypePtr>
class UniqueID : public safe_bool<UniqueID<ResourceTypePtr> >{
public:
    typedef ResourceTypePtr resource_pointer_type;

    typedef std::function<ResourceTypePtr (const UniqueID<ResourceTypePtr>*)> ResourceGetter;

    UniqueID(const UniqueID<ResourceTypePtr>& other):
        id_(other.id_),
        getter_(other.getter_) {

    }

    UniqueID(ResourceGetter getter=ResourceGetter()):
        id_(0),
        getter_(getter) {}

    ResourceTypePtr fetch() const {
        assert(is_bound() && "This ID is not bound to a resource manager");
        return getter_(this);
    }

    bool is_bound() const {
        return bool(getter_);
    }

    explicit UniqueID(uint32_t id, ResourceGetter getter=ResourceGetter()):
        id_(id),
        getter_(getter) {

    }

    bool operator==(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ == other.id_;
    }

    UniqueID& operator=(const UniqueID<ResourceTypePtr>& other) {
        if(&other == this) {
            return *this;
        }

        this->id_ = other.id_;
        this->getter_ = other.getter_;
        return *this;
    }

    bool operator<(const UniqueID<ResourceTypePtr>& other) const {
        return this->id_ < other.id_;
    }

    bool operator!=(const UniqueID<ResourceTypePtr>& other) const {
        return !(*this == other);
    }

    friend std::ostream& operator<< (std::ostream& o, UniqueID<ResourceTypePtr> const& instance) {
        return o << instance.value();
    }

    bool boolean_test() const {
        return id_ != 0;
    }

    uint32_t value() const { return id_; }

private:
    uint32_t id_ = 0;
    ResourceGetter getter_;
};

namespace std {
    template<typename ResourcePtrType>
    struct hash< UniqueID<ResourcePtrType> > {
        size_t operator()(const UniqueID<ResourcePtrType>& id) const {
            hash<uint32_t> make_hash;
            return make_hash(id.value());
        }
    };
}


#endif // UNIQUE_ID_H
