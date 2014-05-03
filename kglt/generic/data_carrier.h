#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

#include <unordered_map>
#include <boost/any.hpp>
#include <kazbase/exceptions.h>
#include <kazbase/list_utils.h>

namespace kglt {
namespace generic {

class DataCarrier {
public:
    virtual ~DataCarrier() {}

    template<typename T>
    void stash(T thing, const std::string& identifier) {
        things_[identifier] = boost::any(thing);
    }

    bool exists(const std::string& identifier) const {
        return container::contains(things_, identifier);
    }

    template<typename T>
    T get(const std::string& identifier) const {
        if(!exists(identifier)) {
            throw DoesNotExist<T>();
        }
        return boost::any_cast<T>(things_.at(identifier));
    }

    void unstash(const std::string& identifier) {
        if(!exists(identifier)) return;

        things_.erase(identifier);
    }

private:
    std::unordered_map<std::string, boost::any> things_;
};

}
}

#endif // USER_DATA_CARRIER_H
