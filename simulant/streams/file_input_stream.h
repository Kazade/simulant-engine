#pragma once

#include <cstdio>
#include "base.h"
#include "../utils/unicode.h"

namespace smlt {

class FileInputStream : public Stream {
public:
    FileInputStream(const unicode& filepath) {
        file_in = fopen(filepath.encode().c_str(), "rb");
        if(file_in) {
            set_status(STREAM_STATE_OK);
        } else {
            set_status(STREAM_STATE_FAILED);
        }
    }

    StreamState read(uint8_t* out, std::size_t size) override {
        if(!ready()) {
            return status();
        }

        fread((void*) out, size, 1, file_in);

        if(feof(file_in)) {
            set_status(STREAM_STATE_EOF);
        } else if(ferror(file_in)) {
            set_status(STREAM_STATE_FAILED);
        }

        return status();
    }

    virtual StreamState seek(const std::size_t offset, SeekFrom from=SEEK_FROM_START) override {
        fseek(
            file_in,
            offset,
            (from == SEEK_FROM_START) ? SEEK_SET :
            (from == SEEK_FROM_CURRENT) ? SEEK_CUR : SEEK_END
        );

        if(feof(file_in)) {
            set_status(STREAM_STATE_EOF);
        } else if(ferror(file_in)) {
            set_status(STREAM_STATE_FAILED);
        }

        return status();
    }

    std::size_t tell() const override {
        return ftell(file_in);
    }

    void close() override {
        if(file_in) {
            fclose(file_in);
        }
        set_status(STREAM_STATE_EOF);
    }
private:
    FILE* file_in = nullptr;
};

}
