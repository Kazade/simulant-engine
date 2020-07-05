#pragma once

#include <string>

namespace smlt {

/**
 * @brief The Nameable class
 *
 * Any object that can be given a user-friendly name
 */
class Nameable {
public:
    virtual ~Nameable() {}

    void set_name(const std::string& name) {
        name_ = name;
    }

    const std::string& name() const {
        return name_;
    }

    bool has_name() const {
        return !name_.empty();
    }

private:
    std::string name_;
};

template<typename T>
class ChainNameable:
    public virtual Nameable {

public:
    T* set_name_and_get(const std::string& name) {
        set_name(name);
        return dynamic_cast<T*>(this);
    }
};

}
