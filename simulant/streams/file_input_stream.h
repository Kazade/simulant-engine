#pragma once

#include <cassert>
#include <cstdio>
#include "base.h"
#include "../utils/unicode.h"

namespace smlt {

class FileInputStream : public Stream {
public:
    FileInputStream(const unicode& filepath) {
        file_in = fopen(filepath.encode().c_str(), "rb");
        if(file_in) {
            register_close_func([this]() {
                fclose(file_in);
                file_in = nullptr;
            });

            set_status(STREAM_STATE_OK);
        } else {
            set_status(STREAM_STATE_FAILED);
        }
    }

    StreamState read(uint8_t* out, std::size_t size) override {
        if(!ok()) {
            return status();
        }

        assert(file_in);

        fread((void*) out, size, 1, file_in);

        if(feof(file_in)) {
            set_status(STREAM_STATE_EOF);
        } else if(ferror(file_in)) {
            set_status(STREAM_STATE_FAILED);
        }

        return status();
    }

    virtual StreamState seek(const std::size_t offset, SeekFrom from=SEEK_FROM_START) override {
        if(!ok()) {
            return status();
        }

        if(fseek(
            file_in,
            offset,
            (from == SEEK_FROM_START) ? SEEK_SET :
            (from == SEEK_FROM_CURRENT) ? SEEK_CUR : SEEK_END
        ) != 0) {
            return STREAM_STATE_FAILED;
        }

        if(feof(file_in)) {
            set_status(STREAM_STATE_EOF);
        } else if(ferror(file_in)) {
            set_status(STREAM_STATE_FAILED);
        }

        return status();
    }

    std::size_t tell() const override {
        assert(file_in);

        return ftell(file_in);
    }
private:
    FILE* file_in = nullptr;
};

}
