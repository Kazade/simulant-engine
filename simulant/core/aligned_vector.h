#pragma once

#include <vector>
#include "aligned_allocator.h"

namespace smlt {

template<typename T, int Align>
using aligned_vector = std::vector<T, aligned_allocator<T, Align>>;

}
