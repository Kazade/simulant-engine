#ifdef _arch_dreamcast
#include <kos.h>
#endif

#include "memory.h"
#include "../deps/kazlog/kazlog.h"

namespace smlt {

void print_available_ram() {
#ifdef _arch_dreamcast
    malloc_stats();
#endif
}


}
