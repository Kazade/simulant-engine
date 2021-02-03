#pragma once

#include <memory>
#include <istream>

namespace smlt {

/* StreamView is like a proxy to an underlying input stream. You can
 * have multiple StreamViews sharing the same input stream, and each maintains
 * its position in the stream for read operations.
 *
 * This is mainly for dealing with multiple audio streams using the same
 * underlying audio file.
 *
 * This is a pseudo-istream. Perhaps at some point we can convert this to an
 * actual istream/streambuf.
 */

class StreamView {
public:
    StreamView(std::shared_ptr<std::istream> stream):
        stream_(stream) {

        auto g = stream_->tellg();
        stream_->seekg(0, std::ios_base::end);
        stream_size_ = stream_->tellg();
        stream_->seekg(g, std::ios_base::beg);
    }

    StreamView& seekg(std::streamoff pos, std::ios_base::seekdir way) {
        if(way == std::ios_base::beg) {
            cursor_ = pos;
        } else if(way == std::ios_base::cur) {
            cursor_ += pos;
        } else {
            cursor_ = stream_size_ - pos;
        }
    }

    StreamView& read(char* s, std::streamsize n) {
        auto g = stream_->tellg(); // push

        stream_->seekg(cursor_);
        stream_->read(s, n);
        cursor_ += n;

        stream_->seekg(g);  // pop
    }

private:
    std::shared_ptr<std::istream> stream_;
    std::streampos cursor_;
    std::size_t stream_size_;
};


}
