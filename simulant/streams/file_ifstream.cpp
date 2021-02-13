#include "file_ifstream.h"

namespace smlt {

FileStreamBuf::int_type FileStreamBuf::underflow() {
    if(feof(filein_)) {
        return traits_type::eof();
    }

    char c = fgetc(filein_);

    if(feof(filein_)) {
        return traits_type::eof();
    }

    fseek(filein_, -1, SEEK_CUR);
    return traits_type::to_int_type(c);
}

FileStreamBuf::int_type FileStreamBuf::uflow() {
    if(feof(filein_)) {
        return traits_type::eof();
    }

    char c = fgetc(filein_);

    if(feof(filein_)) {
        return traits_type::eof();
    }

    return traits_type::to_int_type(c);
}

std::streampos FileStreamBuf::seekpos(std::streampos sp, std::ios_base::openmode which) {
    _S_UNUSED(which);
    assert(which & std::ios_base::in);

    fseek(filein_, sp, SEEK_SET);

    return sp;
}

std::streampos FileStreamBuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
    _S_UNUSED(which);
    assert(which & std::ios_base::in);

    fseek(
        filein_, off,
        (way == std::ios_base::beg) ? SEEK_SET :
        (way == std::ios_base::cur) ? SEEK_CUR : SEEK_END
    );

    return pos_type(off_type(ftell(filein_)));
}

FileStreamBuf::int_type FileStreamBuf::pbackfail(FileStreamBuf::int_type c) {
    fseek(filein_, -1, SEEK_CUR);
    return traits_type::to_int_type(c);
}

FileStreamBuf::int_type FileStreamBuf::sbumpc() {
    return uflow();
}

FileStreamBuf::int_type FileStreamBuf::sgetc() {
    return underflow();
}

}
