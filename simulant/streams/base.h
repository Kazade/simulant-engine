#pragma once

#include <functional>
#include <cstdint>
#include <memory>
#include <string>

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
    Stream() = default;
    Stream(const Stream&) = delete;
    Stream& operator=(const Stream&) = delete;

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

    virtual StreamState peek(uint8_t* c) {
        if(!ok()) {
            return status();
        }

        read(c, 1);
        bool was_eof = eof();
        seek(-1, SEEK_FROM_CURRENT);
        return (was_eof) ? STREAM_STATE_EOF : status_;
    }

    virtual StreamState write(const std::string& s) {
        for(auto c: s) {
            write((uint8_t*) &c, 1);
        }

        return status_;
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

    virtual std::size_t tell() const {
        return 0;
    }

    virtual void close() {
        if(close_func_) {
            close_func_();
        }
        status_ = STREAM_STATE_EOF;
    }

    StreamState status() const {
        return status_;
    }

    bool ok() const {
        return status_ == STREAM_STATE_OK;
    }

    bool eof() const {
        return status_ == STREAM_STATE_EOF;
    }

    bool failed() const {
        return status_ == STREAM_STATE_FAILED;
    }

    operator bool() const {
        return ok();
    }

protected:
    void set_status(StreamState status) {
        status_ = status;
    }

    template<typename T>
    void register_close_func(T func) {
        close_func_ = func;
    }
private:
    StreamState status_ = STREAM_STATE_EOF;

    std::function<void ()> close_func_;
};

typedef std::shared_ptr<Stream> StreamPtr;

}
