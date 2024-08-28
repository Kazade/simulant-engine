#include <ostream>

#include "path.h"
#include "utils/kfs.h"

namespace smlt {

std::ostream& operator<<(std::ostream& os, const Path& p) {
    os << p.str();
    return os;
}

Path Path::system_temp_dir() {
#ifdef WIN32
    TCHAR temp_path_buffer[MAX_PATH];
    GetTempPath(MAX_PATH, temp_path_buffer);
    return std::string(temp_path_buffer);
#elif __DREAMCAST__
    /* KallistiOS mounts a ram disk at /ram. Given that
     * we have no writeable memory on the DC (aside the VMU)
     * this is the best we got */
    return "/ram";
#else
    return "/tmp";
#endif
}

Path Path::parent() const {
    return kfs::path::dir_name(path_);
}

Path Path::normalize() const {
    return kfs::path::norm_path(path_);
}

std::pair<Path, std::string> Path::split() const {
    auto p = kfs::path::split(path_);
    auto p0 = Path(p.first);
    return std::make_pair(p0, p.second);
}

std::string Path::ext() const {
    return kfs::path::split_ext(path_).second;
}

bool Path::is_absolute() const {
    return kfs::path::is_absolute(path_);
}

Path Path::replace_ext(const std::string &new_ext) const {
    std::string prefix = path_.substr(0, path_.find_last_of("."));

    auto ext = new_ext;

    if(!ext.empty() && ext[0] == '.') {
        ext = ext.substr(1, std::string::npos);
    }

    return Path(prefix + "." + ext);
}

std::string Path::name() const {
    return split().second;
}

Path Path::absoluted() const {
    return kfs::path::abs_path(path_);
}

Path Path::append(const std::string &name) const {
    return kfs::path::join(path_, name);
}

bool Path::operator==(const Path& p) const {
    return path_ == p.path_;
}

bool Path::operator<(const Path &p) const {
    return path_ < p.path_;
}

}
