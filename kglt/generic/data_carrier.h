#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

#include <unordered_map>
#include <kazbase/any/any.h>
#include <kazbase/exceptions.h>

namespace kglt {
namespace generic {

class DataCarrier {
public:
    virtual ~DataCarrier() {}

    template<typename T>
    void stash(T thing, const std::string& identifier) {
        things_[identifier] = kazbase::any(thing);
    }

    bool exists(const std::string& identifier) const {
        return things_.count(identifier);
    }

    template<typename T>
    T get(const std::string& identifier) const {
        if(!exists(identifier)) {
            throw DoesNotExist<T>();
        }
        return kazbase::any_cast<T>(things_.at(identifier));
    }

    void unstash(const std::string& identifier) {
        if(!exists(identifier)) return;

        things_.erase(identifier);
    }

private:
    std::unordered_map<std::string, kazbase::any> things_;
};

}
}

#endif // USER_DATA_CARRIER_H
