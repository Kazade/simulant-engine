#pragma once

#include "../threads/atomic.h"

/* Adds a uuid() method to objects which is distinct
 * across objects of this type. Copying / assigning the object will
 * generate a new uuid! */

namespace smlt {

typedef uint64_t uuid64;

template<typename T>
class UniquelyIdentifiable {
public:
    UniquelyIdentifiable():
        uuid_(generate_uuid()) {}

    UniquelyIdentifiable(const UniquelyIdentifiable& rhs):
        uuid_(generate_uuid()) {}

    UniquelyIdentifiable& operator=(const UniquelyIdentifiable& rhs) {
        if(&rhs == this) {
            return *this;
        }

        uuid_ = generate_uuid();
        return *this;
    }

    uuid64 uuid() const {
        return uuid_;
    }

private:
    /* FIXME: Do something better */
    static uuid64 generate_uuid() {
        static thread::Atomic<uuid64> counter(~0);
        return --counter;
    }

    uuid64 uuid_;
};

}
