#include "stream.h"

namespace smlt {

bool get_line(StreamPtr stream, std::string& line) {
    line.clear();

    char c = '\0';
    while(stream->ok()) {
        if(stream->read(&c, 1) != STREAM_STATE_OK) {
            break;
        }

        if(c == '\n') {
            break;
        }

        line += c;
    }

    return stream->status() != STREAM_STATE_EOF;
}

StreamPtr open(const std::string& data) {
    StreamPtr stream = std::make_shared<MemoryStream>();
    stream->write(data);
    stream->seek(0);
    return stream;
}

StreamPtr open(uint8_t* data, uint32_t size, StreamMode mode) {
    L_ERROR("Not implemented: data-streams");
    return StreamPtr();
}

StreamPtr open(const unicode& filename, StreamMode mode) {
    if(filename == MEMORY_FILE) {
        return std::make_shared<MemoryStream>();
    } else if(mode == STREAM_MODE_READ) {
        auto stream = std::make_shared<FileInputStream>(filename);
        if(!stream->ok()) {
            return nullptr;
        } else {
            return stream;
        }
    } else {
        L_ERROR("Not implemented: write-streams");
        return StreamPtr();
    }
}

std::size_t read_into(StreamPtr stream, std::string& str) {
    assert(stream);

    stream->seek(0);

    std::size_t count = 0;
    while(stream->ok()) {
        uint8_t c;
        if(stream->read(&c, 1) == STREAM_STATE_OK) {
            str += c;
            ++count;
        }
    }
    return count;
}

std::size_t read_into(StreamPtr stream, std::vector<uint8_t>& bytes) {
    while(stream->ok()) {
        uint8_t c;
        if(stream->read(&c, 1) == STREAM_STATE_OK) {
            bytes.push_back(c);
        }
    }
    return bytes.size();
}

}
