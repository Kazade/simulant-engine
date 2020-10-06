#pragma once

#include <streambuf>
#include <algorithm>
#include "base.h"

namespace smlt {

struct StreamBuf : public std::streambuf {
    StreamBuf(StreamPtr stream):
        stream_(stream) {

    }

    int underflow() override {
        uint8_t c;
        if(stream_->peek(&c) == STREAM_STATE_EOF) {
            return EOF;
        }

        return (int) c;
    }

    int uflow() override {
        uint8_t c;
        stream_->read(&c, 1);
        return (stream_->eof()) ? EOF : (int) c;
    }

    std::streamsize xsgetn(char* s, std::streamsize n)  override {
        auto g = stream_->tell();
        stream_->seek(0, SEEK_FROM_END);
        auto len = stream_->tell();
        stream_->seek(g, SEEK_FROM_START);
        stream_->read((uint8_t*) s, n);

        return std::min<std::streamsize>((std::streamsize) len - g, n);
    }

    StreamPtr stream_;
};

}
