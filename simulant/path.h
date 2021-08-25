#pragma once

#include <string>

namespace smlt {

class Path {
public:
    Path() = default;

    Path(const char* path):
        path_(path) {}

    Path(const std::string& path):
        path_(path) {}

    std::string str() const {
        return path_;
    }

    std::string ext() const;

    /* Returns a new Path object, with the extension replaced. If there is no
     * extension this will effectively append the extension to the path. */
    Path replace_ext(const std::string& new_ext) const {
        std::string prefix = path_.substr(0, path_.find_last_of("."));

        auto ext = new_ext;

        if(!ext.empty() && ext[0] == '.') {
            ext = ext.substr(1, std::string::npos);
        }

        return Path(prefix + "." + ext);
    }

    bool operator==(const Path& p) const;

private:
    std::string path_;
};

std::ostream& operator<<(std::ostream& os, const Path& p);

}

namespace std {

template<>
struct hash<smlt::Path> {
    std::size_t operator()(const smlt::Path& k) const {
        return hash<std::string>()(k.str());
    }
};

}
