#pragma once

#include <cstdint>
#include <cstdlib>

namespace smlt {

void* aligned_alloc(std::size_t alignment, std::size_t size);

}
