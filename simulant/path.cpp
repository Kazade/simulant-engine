#include "path.h"
#include "utils/kfs.h"

namespace smlt {

std::ostream& operator<<(std::ostream& os, const Path& p) {
    os << p.str();
    return os;
}

Path Path::parent() const {
    return kfs::path::dir_name(path_);
}

std::string Path::ext() const {
    return kfs::path::split_ext(path_).second;
}

bool Path::operator==(const Path& p) const {
    return path_ == p.path_;
}

bool Path::operator<(const Path &p) const {
    return path_ < p.path_;
}

}
