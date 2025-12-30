#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <sstream>

std::shared_ptr<std::istream> stream_from_memory(const uint8_t* data,
                                                 const std::size_t size);
