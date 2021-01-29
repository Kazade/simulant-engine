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
public:
    FileStreamBuf(const std::string& name, const std::string& mode) {
        filein_ = fopen(name.c_str(), mode.c_str());
        assert(filein_);
    }

    ~FileStreamBuf() {
        fclose(filein_);
    }

    int_type underflow() override;

    int_type uflow() override;

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

private:
    std::shared_ptr<FileStreamBuf> buffer_;
};


}
