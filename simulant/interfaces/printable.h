#pragma once

#include "../utils/unicode.h"

namespace smlt {

class Printable {
public:
    virtual ~Printable() {}

    virtual unicode to_unicode() const = 0;

    friend std::ostream& operator<< (std::ostream& o, Printable const& instance);
};

std::ostream& operator<< (std::ostream& o, Printable const& instance);


}
