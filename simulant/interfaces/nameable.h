#pragma once

#include "../utils/limited_string.h"
#include <string>

namespace smlt {

/**
 * @brief The Nameable class
 *
 * Any object that can be given a user-friendly name
 */
class Nameable {
public:
    static constexpr std::size_t MAX_NAME_LENGTH = 64;

    virtual ~Nameable() {}

    void set_name(const char* name) {
        name_ = name;
    }

    void set_name(const std::string& name) {
        name_ = name;
    }

    void set_name(const LimitedString<MAX_NAME_LENGTH>& name) {
        name_ = name;
    }

    std::string name() const {
        return name_.str();
    }

    bool has_name() const {
        return !name_.empty();
    }

private:
    LimitedString<MAX_NAME_LENGTH> name_;
};

template<typename T>
class ChainNameable:
    public virtual Nameable {

public:
    T* set_name_and_get(const LimitedString<MAX_NAME_LENGTH>& name) {
        set_name(name);
        return dynamic_cast<T*>(this);
    }
};

}
