#pragma once

#include <istream>

#include "base.h"

namespace smlt {

class IStream : public std::istream {
public:
    IStream(StreamPtr stream):
        stream_(stream) {}

private:
    StreamPtr stream_;
};

}
