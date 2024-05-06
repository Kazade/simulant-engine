#pragma once

#include <cstdint>

std::size_t generate_mipmap_level_rgb565(int lw, int lh, const uint8_t* src,
                                         uint8_t* dest);

std::size_t generate_mipmap_level_rgba4444(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest);

std::size_t generate_mipmap_level_rgba5551(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest);

std::size_t generate_mipmap_level_rgba8888(int lw, int lh, const uint8_t* src,
                                           uint8_t* dest);
