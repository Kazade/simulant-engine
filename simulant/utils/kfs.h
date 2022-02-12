#pragma once

#include <vector>
#include <string>
#include <utility>
#include <stdexcept>
#include <cstdint>

#ifdef __WIN32__
    //#error "Must implement windows support";
#else
    #include <sys/stat.h>
#endif

namespace kfs {

class IOError: public std::runtime_error {
public:
    IOError(const std::string& what):
        std::runtime_error(what) {}

    IOError(int err):
        std::runtime_error(IOError::get_message(err)),
        err(err) {

    }

    int err = 0;
private:
    static std::string get_message(int err);
};

#ifdef __WIN32__
    const std::string SEP = "\\";
#else
    const std::string SEP = "/";
#endif

typedef std::string Path;
typedef uint32_t Mode;

#ifdef __WIN32__
    // Windows doesn't support Posix
    typedef uint32_t nlink_t;
    typedef uint32_t uid_t;
    typedef uint32_t gid_t;
#endif

struct Stat {
    dev_t     dev;     /* ID of device containing file */
    ino_t     ino;     /* inode number */
    mode_t    mode;    /* protection */
    nlink_t   nlink;   /* number of hard links */
    uid_t     uid;     /* user ID of owner */
    gid_t     gid;     /* group ID of owner */
    dev_t     rdev;    /* device ID (if special file) */
    off_t     size;    /* total size, in bytes */
    uint32_t  atime;   /* time of last access */
    uint32_t  mtime;   /* time of last modification */
    uint32_t  ctime;   /* time of last status change */
};

std::pair<Stat, bool> lstat(const Path& path);

void touch(const Path& path);
void rename(const Path& old, const std::string& new_path);

void remove(const Path& path);
void remove_dir(const Path& path);
void remove_dirs(const Path& path);

void make_dir(const Path& path, Mode mode=0777);
void make_dirs(const Path& path, Mode mode=0777);
void make_link(const Path& source, const Path& dest);

Path temp_dir();

Path exe_path();
Path exe_dirname();
Path get_cwd();

namespace path {

    Path join(const Path& p1, const Path& p2);
    Path join(const std::vector<Path>& parts);

    Path abs_path(const Path& p);
    Path norm_path(const Path& path);
    Path norm_case(const Path& path);


    bool exists(const Path& path);
    Path dir_name(const Path &path);
    bool is_absolute(const Path& path);
    bool is_dir(const Path& path);
    bool is_file(const Path& path);
    bool is_link(const Path& path);

    Path real_path(const Path& path);

    void hide_dir(const Path& path);
    std::pair<Path, Path> split_ext(const Path& path);
    Path rel_path(const Path& path, const Path& start=Path());
    Path expand_user(const Path& path);
    std::vector<Path> list_dir(const Path& path);

    std::pair<Path, Path> split(const Path &path);
    std::pair<Path, Path> split_ext(const Path& path);
}

}
