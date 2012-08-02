#ifndef USER_DATA_CARRIER_H
#define USER_DATA_CARRIER_H

#include <boost/any.hpp>

namespace kglt {
namespace generic {

class UserDataCarrier {
public:
    virtual ~UserDataCarrier() {}

    void set_user_data(boost::any data) {
        user_data_ = data;
    }

    template<typename T>
    T user_data() const {
        assert(!user_data_.empty());
        return boost::any_cast<T>(user_data_);
    }

    bool has_user_data() const { return !user_data_.empty(); }
private:
    boost::any user_data_;
};

}
}

#endif // USER_DATA_CARRIER_H
