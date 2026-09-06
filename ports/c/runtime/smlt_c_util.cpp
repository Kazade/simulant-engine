#include "smlt_c_util.h"

#include <cstdlib>
#include <cstring>

extern "C" char* smlt_c_strdup(const char* s) {
    if (!s) {
        return nullptr;
    }

    std::size_t len = std::strlen(s) + 1;
    char* out = static_cast<char*>(std::malloc(len));
    if (out) {
        std::memcpy(out, s, len);
    }
    return out;
}

extern "C" void smlt_c_free_string(char* str) {
    std::free(str);
}
