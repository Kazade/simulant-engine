#pragma once

#include <cstdint>
#include <string>

namespace sprite_gen {

/* Writes an 8-bit RGBA PNG. Only used internally to hand the sheet's pixels
 * to texconv for -C/--compress - texconv (via Qt) is guaranteed to read PNG,
 * whereas TGA support is an optional Qt plugin that may not be installed.
 * `rgba` must contain width * height * 4 bytes, row-major, top-to-bottom,
 * RGBA byte order. Returns false (and logs an error) if the file couldn't be
 * written. */
bool write_png(const std::string& path, uint16_t width, uint16_t height,
               const uint8_t* rgba);

} // namespace sprite_gen
