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

    /** Returns the parent path of this one. If this is a path to a
     *  directory, it'll return the parent directory. If it's a path
     *  to a file (e.g. name() is not empty) then it will return the
     *  directory containing the file.
     */
    Path parent() const;

    /** Normalizes a path, replacing things like '..' */
    Path normalize() const;

    /** Returns a pair of parent() and name() */
    std::pair<Path, std::string> split() const;

    /** Returns the name of the path if it's not a directory
     *  path, otherwise returns an empty string */
    std::string name() const;

    Path absoluted() const;

    Path append(const std::string& name) const;

    /** Convert the full path to a std::string */
    std::string str() const {
        return path_;
    }

    std::string ext() const;

    /* Returns a new Path object, with the extension replaced. If there is no
     * extension this will effectively append the extension to the path. */
    Path replace_ext(const std::string& new_ext) const;

    bool operator==(const Path& p) const;
    bool operator<(const Path& p) const;
    bool operator!=(const Path& p) const {
        return !(*this == p);
    }

    /** Returns true if this is an absolute path. On systems
     *  with a Unix-like filesystem this returns true if the path
     *  starts with a '/'. On Windows this will return true if the
     *  normalized path starts with a '/' with the drive letter removed
     */
    bool is_absolute() const;
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
