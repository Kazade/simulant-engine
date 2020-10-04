#pragma once

#include <cstdint>

#include "../utils/unicode.h"
#include "base.h"
#include "file_input_stream.h"
#include "null_stream.h"
#include "istream.h"

namespace smlt {

enum StreamMode {
    STREAM_MODE_READ,
    STREAM_MODE_WRITE
};

StreamPtr open(const unicode& filename, StreamMode mode);
StreamPtr open(uint8_t* data, uint32_t size, StreamMode mode);

std::size_t read_into(StreamPtr stream, std::vector<uint8_t>& bytes);
std::size_t read_into(StreamPtr stream, std::string& str);

bool get_line(StreamPtr stream, std::string& line) {
    char c = '\0';
    while(c != '\n' && stream->ready()) {
        stream->read(&c, 1);
        line += c;
    }

    return stream->status() != STREAM_STATE_EOF;
}

}
