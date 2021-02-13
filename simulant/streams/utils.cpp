#include "utils.h"

namespace smlt {

bool readline(std::istream &in, std::string &out) {
    out.clear();
    while(in.good()) {
        char c;
        in.get(c);
        if(c == '\n') {
            break;
        } else {
            out += c;
        }
    }

    return in.good() && !out.empty();
}

}
