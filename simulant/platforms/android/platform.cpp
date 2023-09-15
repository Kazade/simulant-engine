#include "platform.h"

namespace smlt {

std::string AndroidPlatform::name() const {
    return "android";
}

smlt::Resolution smlt::AndroidPlatform::native_resolution() const {
    Resolution native;

    return native;
}


}
