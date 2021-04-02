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
