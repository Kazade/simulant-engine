#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

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

    bool exists(const std::string& identifier) {
        return container::contains(things_, identifier);
    }

    template<typename T>
    T get(const std::string& identifier) {
        if(!exists(identifier)) {
            throw DoesNotExist<T>();
        }
        return boost::any_cast<T>(things_[identifier]);
    }

    void unstash(const std::string& identifier) {
        if(!exists(identifier)) return;

        things_.erase(identifier);
    }

private:
    std::map<std::string, boost::any> things_;
};

}
}

#endif // USER_DATA_CARRIER_H
