#pragma once

#include <cstdint>
#include <string>

namespace sprite_gen {

/* Writes an uncompressed 32bpp (RGBA) TGA file. `rgba` must contain
 * width * height * 4 bytes, row-major, top-to-bottom, RGBA byte order.
 * Returns false (and logs an error) if the file couldn't be written. */
bool write_tga(const std::string& path, uint16_t width, uint16_t height,
               const uint8_t* rgba);

} // namespace sprite_gen
