#include "file_ifstream.h"
#include "../logging.h"

namespace smlt {

uint32_t FileStreamBuf::open_file_counter = 0;

FileStreamBuf::FileStreamBuf(const std::string &name, const std::string &mode) {
    ++open_file_counter;
    filein_ = fopen(name.c_str(), mode.c_str());
    assert(filein_);

    if(open_file_counter == FILE_OPEN_WARN_COUNT) {
        S_WARN(
            "{0} files are concurrently open, this may cause issues on some platforms",
            open_file_counter
        );
    }
    setg(buffer_, buffer_, buffer_);
}

FileStreamBuf::~FileStreamBuf() {
    fclose(filein_);
    --open_file_counter;
}

FileStreamBuf::int_type FileStreamBuf::underflow() {
    auto old_read_pos = last_read_pos_;
    last_read_pos_ = ftell(filein_);
    assert(!ferror(filein_));
    auto bytes = fread(buffer_, sizeof(char), BUFFER_SIZE, filein_);

    if(!bytes) {
        last_read_pos_ = old_read_pos;
        return traits_type::eof();
    } else {
        setg(buffer_, buffer_, buffer_ + bytes);
        return traits_type::to_int_type(*gptr());
    }
}

std::streampos FileStreamBuf::seekpos(std::streampos sp, std::ios_base::openmode which) {
    _S_UNUSED(which);
    assert(which & std::ios_base::in);

    fseek(filein_, sp, SEEK_SET);

    // Invalidate the buffer so underflow will be called
    setg(buffer_, buffer_, buffer_);

    return sp;
}

std::streampos FileStreamBuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
    _S_UNUSED(which);
    assert(which & std::ios_base::in);

    if(way == std::ios_base::cur) {
        /* FIXME: No-op if still within the buffer? */
        auto dist_in_buffer = gptr() - eback();
        auto current_pos = last_read_pos_ + dist_in_buffer;

        if(off == 0) {
            /* Don't do anything, just return the current position
             * this saves unnecessary reads for tellg() which is implemented
             * by calling this with off == 0 */
            return pos_type(off_type(current_pos));
        }

        auto abs_pos = current_pos + off;

        if(abs_pos < last_read_pos_ || gptr() + off >= egptr()) {
            fseek(filein_, abs_pos, std::ios::beg);
            setg(buffer_, buffer_, buffer_);
            underflow();
            return pos_type(off_type(last_read_pos_));
        } else {
            /* We're still within the buffer, so nothing to do
             * except manipulate the gptr() */
            setg(buffer_, gptr() + off, egptr());
            return pos_type(off_type(abs_pos));
        }

    } else if(way == std::ios_base::end && off == 0) {
        /* Optimisation: seeking to the end, don't do a read, just fseek there and return the
         * result of ftell */
        fseek(filein_, 0, SEEK_END);
        last_read_pos_ = ftell(filein_);
        setg(buffer_, buffer_, buffer_);
        return pos_type(off_type(last_read_pos_));
    }

    /* Even if we're using std::ios_base::cur, we now have an absolute offset */
    fseek(
        filein_, off,
        (way == std::ios_base::beg) ? SEEK_SET :
        (way == std::ios_base::cur) ? SEEK_SET : SEEK_END
    );

    // Invalidate the buffer so underflow will be called
    setg(buffer_, buffer_, buffer_);

    return pos_type(off_type(ftell(filein_)));
}

FileStreamBuf::int_type FileStreamBuf::pbackfail(FileStreamBuf::int_type c) {
    /* This is called in the event that someone calls unget() or whatever, and there's
     * no buffer space left to put back. */

    /* Move back one char and re-read */
    fseek(filein_, last_read_pos_ - 1, SEEK_SET);
    auto bytes = fread(buffer_, sizeof(char), BUFFER_SIZE, filein_);

    /* Update the buffer */
    setg(buffer_, buffer_, buffer_ + bytes);

    /* Update the last read position */
    last_read_pos_--;

    /* Finally, if we weren't putting back EOF, then but that into the buffer */
    if(c != EOF) {
        *gptr() = c;
    }

    return traits_type::to_int_type(*gptr());
}

}
