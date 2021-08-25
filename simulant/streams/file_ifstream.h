#pragma once

#include <istream>
#include <streambuf>
#include <memory>
#include <cstdio>
#include <cassert>

#include "../macros.h"

namespace smlt {

class FileStreamBuf : public std::streambuf {
    /* This is essentially a std::istream wrapper around FILE*
     * mainly so that we can access the underlying FILE* for C
     * APIs (e.g. stb_vorbis) */

    static const int BUFFER_SIZE = 4096;

    /* This is used to log a warning when we start having quite
     * a lot of files open. Some platforms (Dreamcast) are limited
     * in how many open files there can be... */

    static const uint32_t FILE_OPEN_WARN_COUNT = 6;
    static uint32_t open_file_counter;
public:
    FileStreamBuf(const std::string& name, const std::string& mode);
    ~FileStreamBuf();

    int_type underflow() override;

    std::streampos seekpos(
        std::streampos sp,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

    std::streampos seekoff(
        std::streamoff off,
        std::ios_base::seekdir way,
        std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;

    int_type pbackfail(int_type c = EOF) override;

    FILE* file() const {
        return filein_;
    }

private:
    FILE* filein_ = nullptr;
    char buffer_[BUFFER_SIZE];
    uint32_t last_read_pos_ = 0;
};

class FileIfstream : public std::istream {
public:
    FileIfstream(std::shared_ptr<FileStreamBuf> buf):
        std::istream(buf.get()),
        buffer_(buf) {

    }

    FILE* file() const {
        return buffer_->file();
    }

    explicit operator bool() const {
        return !fail();
    }

private:
    std::shared_ptr<FileStreamBuf> buffer_;
};


}
