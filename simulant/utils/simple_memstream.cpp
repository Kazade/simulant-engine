#include "simple_memstream.h"

std::shared_ptr<std::istream> stream_from_memory(const uint8_t* data,
                                                 const std::size_t size) {

    // FIXME: This makes a copy of the data, it would be nice if it didn't
    return std::make_shared<std::istringstream>(std::string(data, data + size));
}
