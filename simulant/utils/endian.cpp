#include "endian.h"

#ifdef _MSC_VER
#include <intrin.h>

#define SWAP_16 _byteswap_ushort
#define SWAP_32 _byteswap_ulong
#define SWAP_64 _byteswap_uint64

#elif defined(_arch_dreamcast)
#include <kos.h>
#define SWAP_16 arch_swap16
#define SWAP_32 arch_swap32

#else

#define SWAP_16 __builtin_bswap16
#define SWAP_32 __builtin_bswap32
#define SWAP_64 __builtin_bswap64

#endif

namespace smlt {

static const int16_t num = 1;

bool is_little_endian() {
    return *(char *)&num == 1;
}

uint16_t swap_endian(uint16_t v) {
    return SWAP_16(v);
}

uint32_t swap_endian(uint32_t v) {
    return SWAP_32(v);
}

#ifndef _arch_dreamcast
uint64_t swap_endian(uint64_t v) {
    return SWAP_64(v);
}
#endif

int16_t swap_endian(int16_t v) {
    return SWAP_16(v);
}

int32_t swap_endian(int32_t v) {
    return SWAP_32(v);
}

#ifndef _arch_dreamcast
int64_t swap_endian(int64_t v) {
    return SWAP_64(v);
}
#endif

template<typename T>
T ensure_big(T v) {
    if(is_little_endian()) {
        v = swap_endian(v);
    }
    return v;
}

uint16_t ensure_big_endian(uint16_t v) {
    return ensure_big(v);
}

uint32_t ensure_big_endian(uint32_t v) {
    return ensure_big(v);
}

uint64_t ensure_big_endian(uint64_t v) {
    return ensure_big(v);
}

int16_t ensure_big_endian(int16_t v) {
    return ensure_big(v);
}

int32_t ensure_big_endian(int32_t v) {
    return ensure_big(v);
}

#ifndef _arch_dreamcast
int64_t ensure_big_endian(int64_t v) {
    return ensure_big(v);
}
#endif

}
