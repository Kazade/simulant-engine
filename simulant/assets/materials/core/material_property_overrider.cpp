#include "material_property_overrider.h"
#include "../../../logging.h"
#include "../../../macros.h"

namespace smlt {

bool valid_name(const char* name) {
    const char* i = name;
    while(*i) {
        const char c = *i++;
        if(c >= 'A' && c <= 'Z') continue;
        if(c >= 'a' && c <= 'z') continue;
        if(c >= '0' && c <= '9') continue;
        if(c == '_' || c == '[' || c == ']') continue;

        S_DEBUG("Invalid char: {0}", c);
        return false;
    }

    return true;
}
}
