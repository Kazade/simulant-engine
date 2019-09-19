#pragma once

#include <string>

namespace smlt {

class Printable {
public:
    virtual ~Printable() {}

    virtual std::string repr() const = 0;

    friend std::ostream& operator<< (std::ostream& o, Printable const& instance);
};

std::ostream& operator<< (std::ostream& o, Printable const& instance);


}
