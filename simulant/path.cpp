#include "path.h"
#ifndef _WIN32
#include "deps/kfs/kfs.h"
#else
#include "deps/kfs/kfs/kfs.h"
#endif

namespace smlt {

std::ostream& operator<<(std::ostream& os, const Path& p) {
    os << p.str();
    return os;
}

std::string Path::ext() const {
    return kfs::path::split_ext(path_).second;
}

bool Path::operator==(const Path& p) const {
    return path_ == p.path_;
}

}
