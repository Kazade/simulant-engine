#include <ostream>
#include "printable.h"


namespace smlt {

std::ostream& operator<< (std::ostream& o, Printable const& instance) {
    return o << instance.repr();
}


}
