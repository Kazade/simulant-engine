#include <GL/glkos.h>
#include <dc/pvr.h>
#include <malloc.h>

#include "platform.h"
#include "../../window.h"

static unsigned long systemRam = 0x00000000;
static unsigned long elfOffset = 0x00000000;
static unsigned long stackSize = 0x00000000;

extern uint8_t end[];
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

    /* Widen to 64-bit before subtracting - uordblks/stackSize/systemRam are
     * all 32-bit on this target, and if their sum ever exceeds systemRam
     * (e.g. because stackSize's estimate of the static image size is off)
     * a 32-bit subtraction wraps to a huge value instead of going negative,
     * which then wraps *again* when widened to uint64_t downstream in
     * used_ram_in_bytes() - producing a nonsensical result that prints as a
     * large negative "used RAM" figure. Clamp instead. */
    uint64_t used = (uint64_t)mi.uordblks + (uint64_t)stackSize;
    if(used >= systemRam) {
        return 0;
    }

    return systemRam - used;
}

uint64_t DreamcastPlatform::total_ram_in_bytes() const {
    return systemRam;
}

uint64_t DreamcastPlatform::available_vram_in_bytes() const {
    /* If using the PVR renderer directly, query pvr_mem_available()
     * instead of the GL extension which only works with GLdc */
    uint32_t pvr_avail = pvr_mem_available();
    if(pvr_avail > 0) {
        return (uint64_t) pvr_avail;
    }

    int value;
    glGetIntegerv(GL_FREE_TEXTURE_MEMORY_KOS, &value);
    return (uint64_t) value;
}

uint64_t DreamcastPlatform::process_ram_usage_in_bytes(uint32_t process_id) const {
    _S_UNUSED(process_id);
    return used_ram_in_bytes();
}

}
