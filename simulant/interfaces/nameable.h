#pragma once

#include "../utils/unicode.h"

namespace smlt {

/**
 * @brief The Nameable class
 *
 * Any object that can be given a user-friendly name
 */
class Nameable {
public:
    virtual ~Nameable() {}

    void set_name(const std::string& name) { name_ = name; }
    const std::string& name() const { return name_; }
    bool has_name() const { return !name_.empty(); }
private:
    std::string name_;
};

}
