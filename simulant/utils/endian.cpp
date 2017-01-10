#include "endian.h"

#ifdef _MSC_VER
#include <intrin.h>

#define SWAP_16 _byteswap_ushort
#define SWAP_32 _byteswap_ulong
#define SWAP_64 _byteswap_uint64

#else

#define SWAP_16 __builtin_bswap16
#define SWAP_32 __builtin_bswap32
#define SWAP_64 __builtin_bswap64

#endif

namespace smlt {

union {
    uint16_t s;
    unsigned char c[2];
} constexpr static  d {1};

constexpr bool is_little_endian() {
    return d.c[0] == 1;
}

uint16_t swap_endian(uint16_t v) {
    return SWAP_16(v);
}

uint32_t swap_endian(uint32_t v) {
    return SWAP_32(v);
}

uint64_t swap_endian(uint64_t v) {
    return SWAP_64(v);
}

int16_t swap_endian(int16_t v) {
    return SWAP_16(v);
}

int32_t swap_endian(int32_t v) {
    return SWAP_32(v);
}

int64_t swap_endian(int64_t v) {
    return SWAP_64(v);
}

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

int64_t ensure_big_endian(int64_t v) {
    return ensure_big(v);
}

}
