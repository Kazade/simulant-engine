#pragma once

#include <vector>
#include "base.h"

namespace smlt {

class MemoryStream : public Stream {
public:
    virtual StreamState read(uint8_t* out, std::size_t n) override {
        for(std::size_t i = 0; i < n; ++i) {
            if(cursor_ >= buffer_.size()) {
                set_status(STREAM_STATE_EOF);
                break;
            }

            *out = buffer_[cursor_++];
        }
        return status();
    }

    virtual StreamState peek(uint8_t* c) override {
        if(cursor_ >= buffer_.size()) {
            return STREAM_STATE_EOF;
        }

        *c = buffer_[cursor_ + 1];

        return status();
    }

    virtual StreamState write(const uint8_t* out, std::size_t n) override {
        for(std::size_t i = 0; i < n; ++i) {
            buffer_.push_back(out[i]);
        }

        return status();
    }

    virtual StreamState seek(const std::size_t offset, SeekFrom from=SEEK_FROM_START) override {
        if(from == SEEK_FROM_START) {
            cursor_ = offset;
        } else if(from == SEEK_FROM_CURRENT) {
            cursor_ += offset;
        } else {
            cursor_ -= offset;
        }

        if(cursor_ >= buffer_.size()) {
            cursor_ = buffer_.size();
            set_status(STREAM_STATE_EOF);
        } else {
            set_status(STREAM_STATE_OK);
        }

        return status();
    }

    virtual std::size_t tell() const override {
        return cursor_;
    }
private:
    std::vector<uint8_t> buffer_;
    std::size_t cursor_ = 0;
};

}
