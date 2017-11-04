#pragma once

#include <cstdint>

namespace smlt {

uint16_t swap_endian(uint16_t v);
uint32_t swap_endian(uint32_t v);
uint64_t swap_endian(uint64_t v);
int16_t swap_endian(int16_t v);
int32_t swap_endian(int32_t v);
int64_t swap_endian(int64_t v);

uint16_t ensure_big_endian(uint16_t v);
uint32_t ensure_big_endian(uint32_t v);
uint64_t ensure_big_endian(uint64_t v);
int16_t ensure_big_endian(int16_t v);
int32_t ensure_big_endian(int32_t v);
int64_t ensure_big_endian(int64_t v);


}
