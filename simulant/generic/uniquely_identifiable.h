#pragma once

/* Adds a uuid() method to objects which is distinct
 * across objects of this type. Copying / assigning the object will
 * generate a new uuid! */

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

    uint64_t uuid() const {
        return uuid_;
    }

private:
    /* FIXME: Do something better */
    static uint64_t generate_uuid() {
        static uint64_t counter = ~0;
        return --counter;
    }

    uint64_t uuid_;
};
