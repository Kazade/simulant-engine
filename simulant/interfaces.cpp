#include "interfaces.h"

namespace smlt {

std::ostream& operator<< (std::ostream& o, Printable const& instance) {
    return o << instance.to_unicode().encode();
}


}
