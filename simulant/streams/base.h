#pragma once

#include <cstdint>
#include <memory>

#include "../macros.h"

namespace smlt {

enum StreamState {
    STREAM_STATE_OK,
    STREAM_STATE_EOF,
    STREAM_STATE_FAILED
};

enum SeekFrom {
    SEEK_FROM_START,
    SEEK_FROM_CURRENT,
    SEEK_FROM_END
};

class Stream {
public:
    virtual ~Stream() {
        close();
    }

    virtual StreamState read(uint8_t* out, std::size_t n) {
        _S_UNUSED(out);
        _S_UNUSED(n);
        status_ = STREAM_STATE_FAILED;
        return status_;
    }

    /* Compatibility with the C++ API */
    StreamState read(char* out, std::size_t n) {
        return read((uint8_t*) out, n);
    }

    virtual StreamState write(const uint8_t* out, std::size_t n) {
        _S_UNUSED(out);
        _S_UNUSED(n);
        status_ = STREAM_STATE_FAILED;
        return status_;
    }

    virtual StreamState seek(const std::size_t offset, SeekFrom from=SEEK_FROM_START) {
        _S_UNUSED(offset);
        _S_UNUSED(from);
        status_ = STREAM_STATE_FAILED;
        return status_;
    }

    virtual std::size_t tell() {
        return 0;
    }

    virtual void close() {
        status_ = STREAM_STATE_EOF;
    }

    StreamState status() const {
        return status_;
    }

    bool ready() const {
        return status_ == STREAM_STATE_OK;
    }

    bool eof() const {
        return status_ == STREAM_STATE_EOF;
    }

    bool failed() const {
        return status_ == STREAM_STATE_FAILED;
    }

protected:
    void set_status(StreamState status) {
        status_ = status;
    }

private:
    StreamState status_ = STREAM_STATE_EOF;
};

typedef std::shared_ptr<Stream> StreamPtr;

}
