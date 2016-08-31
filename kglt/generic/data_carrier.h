#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

#include <unordered_map>
#include "any/any.h"

namespace kglt {
namespace generic {

class NoSuchData : public std::runtime_error {
public:
    NoSuchData(const std::string& what):
        std::runtime_error(what) {}
};

class DataCarrier {
public:
    virtual ~DataCarrier() {}

    template<typename T>
    void stash(T thing, const std::string& identifier) {
        things_[identifier] = kglt::any(thing);
    }

    bool exists(const std::string& identifier) const {
        return things_.count(identifier);
    }

    template<typename T>
    T get(const std::string& identifier) const {
        if(!exists(identifier)) {
            throw NoSuchData(identifier);
        }
        return kglt::any_cast<T>(things_.at(identifier));
    }

    void unstash(const std::string& identifier) {
        if(!exists(identifier)) return;

        things_.erase(identifier);
    }

private:
    std::unordered_map<std::string, kglt::any> things_;
};

}
}

#endif // USER_DATA_CARRIER_H
