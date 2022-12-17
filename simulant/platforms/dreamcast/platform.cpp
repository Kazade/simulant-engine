#include <GL/glkos.h>
#include <malloc.h>

#include "platform.h"

static unsigned long systemRam = 0x00000000;
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;

extern unsigned long end;
extern unsigned long start;

#define _end end
#define _start start

namespace smlt {

static void set_system_ram() {
   systemRam = 0x8d000000 - 0x8c000000;
   elfOffset = 0x8c000000;

   stackSize = (long)&_end - (long)&_start + ((long)&_start - elfOffset);
}

uint64_t DreamcastPlatform::available_ram_in_bytes() const {
    if(!systemRam) {
        set_system_ram();
    }

    struct mallinfo mi = mallinfo();
    return systemRam - (mi.uordblks + stackSize);
}

uint64_t DreamcastPlatform::total_ram_in_bytes() const {
    return systemRam;
}

uint64_t DreamcastPlatform::available_vram_in_bytes() const {
    int value;
    glGetIntegerv(GL_FREE_TEXTURE_MEMORY_KOS, &value);
    return (uint64_t) value;
}

uint64_t DreamcastPlatform::process_ram_usage_in_bytes(uint32_t process_id) const {
    _S_UNUSED(process_id);
    return used_ram_in_bytes();
}

}
