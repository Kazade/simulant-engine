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

    void set_name(const unicode& name) { name_ = name; }
    const unicode name() const { return name_; }
    const bool has_name() const { return !name_.empty(); }

    virtual unicode to_unicode() const { return name_; }

private:
    unicode name_;
};

}
