#include <stdlib.h>
#include <malloc.h>
#include "memory.h"


void* smlt::aligned_alloc(std::size_t alignment, std::size_t size) {
    void* p = nullptr;

#ifdef __WIN32__
    p = _aligned_malloc(size, alignment);
#elif defined(__PSP__)
    p = memalign(alignment, size);
#else
    p = ::aligned_alloc(alignment, size);
#endif

    return p;
}
