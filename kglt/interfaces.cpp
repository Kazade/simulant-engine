#include "interfaces.h"

namespace kglt {

std::ostream& operator<< (std::ostream& o, Printable const& instance) {
    return o << instance.__unicode__().encode();
}


}
