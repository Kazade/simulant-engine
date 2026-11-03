#include "platform.h"
#include "../posix.h"

namespace smlt {

std::string AndroidPlatform::name() const {
    return "android";
}

smlt::Resolution AndroidPlatform::native_resolution() const {
    Resolution native;

    return native;
}

User AndroidPlatform::current_user() const {
    return posix::current_user();
}


}
