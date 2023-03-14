#include <malloc.h>

#include "memory.h"

void* smlt::aligned_alloc(std::size_t alignment, std::size_t size) {
    void* p = nullptr;

#ifdef __WIN32__
    p = _aligned_malloc(size, alignment);
#elif defined(__DREAMCAST__)
    p = memalign(alignment, size);
#else
    if(posix_memalign(&p, alignment, size) != 0) {
        p = nullptr;
    }
#endif

    return p;
}
