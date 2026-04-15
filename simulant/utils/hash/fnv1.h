#pragma once

#include <cstdint>

namespace smlt {

template<typename S>
struct fnv_internal;
template<typename S>
struct fnv1;
template<typename S>
struct fnv1a;

template<>
struct fnv_internal<uint32_t> {
    constexpr static uint32_t default_offset_basis = 0x811C9DC5;
    constexpr static uint32_t prime = 0x01000193;
};

template<>
struct fnv1<uint32_t>: public fnv_internal<uint32_t> {
    constexpr static uint32_t hash(const char* str, uint32_t val = default_offset_basis) {
        while(*str) {
            val = (val * prime) ^ uint32_t(uint8_t(*str));
            ++str;
        }
        return val;
    }
};

template<>
struct fnv1a<uint32_t>: public fnv_internal<uint32_t> {
    constexpr static uint32_t hash(const char* str, uint32_t val = default_offset_basis) {
        while(*str) {
            val = (val ^ uint32_t(uint8_t(*str))) * prime;
            ++str;
        }
        return val;
    }
};

} // namespace smlt
