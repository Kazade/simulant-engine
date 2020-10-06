#pragma once

#include <cstdint>

#include "../logging.h"
#include "../utils/unicode.h"
#include "base.h"
#include "file_input_stream.h"
#include "memory_stream.h"
#include "null_stream.h"

namespace smlt {

enum StreamMode {
    STREAM_MODE_READ,
    STREAM_MODE_WRITE
};

StreamPtr open(const unicode& filename, StreamMode mode);
StreamPtr open(uint8_t* data, uint32_t size, StreamMode mode);
StreamPtr open(const std::string& data);

std::size_t read_into(StreamPtr stream, std::vector<uint8_t>& bytes);

std::size_t read_into(StreamPtr stream, std::string& str);

bool get_line(StreamPtr stream, std::string& line);

}
