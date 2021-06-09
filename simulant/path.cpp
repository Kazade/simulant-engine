#include <ostream>

#include "path.h"
#include "deps/kfs/kfs.h"

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
