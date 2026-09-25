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

    /* Construct a view that only exposes a byte range [offset, offset+length)
     * of the underlying stream. Used to stream an embedded chunk (e.g. the
     * `data` chunk of a WAV) without copying it into memory. */
    StreamView(std::shared_ptr<std::istream> stream, std::size_t offset,
               std::size_t length):
        stream_(stream),
        begin_(offset),
        cursor_(offset),
        stream_size_(offset + length) {

        auto g = stream_->tellg();
        stream_->seekg(0, std::ios_base::end);
        auto total = stream_->tellg();
        stream_->seekg(g, std::ios_base::beg);

        /* Clamp to what the underlying stream actually contains. */
        if(total >= 0 &&
           (std::streamoff)(offset + length) > (std::streamoff) total) {
            stream_size_ = (std::size_t) total;
        }
    }

    StreamView& seekg(std::streamoff pos, std::ios_base::seekdir way) {
        if(way == std::ios_base::beg) {
            cursor_ = (std::size_t) (begin_ + pos);
        } else if(way == std::ios_base::cur) {
            cursor_ = (std::size_t) (cursor_ + pos);
        } else {
            cursor_ = (std::size_t) (stream_size_ - pos);
        }

        return *this;
    }

    StreamView& read(char* s, std::streamsize n) {
        auto g = stream_->tellg(); // push

        stream_->seekg(cursor_);
        stream_->read(s, n);
        cursor_ += n;

        stream_->seekg(g);  // pop
        return *this;
    }

    std::size_t length() const {
        return stream_size_ - begin_;
    }

private:
    std::shared_ptr<std::istream> stream_;
    std::streampos begin_ = 0;
    std::streampos cursor_ = 0;
    std::size_t stream_size_ = 0;
};


}
