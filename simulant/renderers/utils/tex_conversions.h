#pragma once

#include <cstdint>
#include <vector>

void tex_convert_argb4444_to_abgr4444(std::vector<uint8_t>& output,
                                      const uint8_t* input, std::size_t w,
                                      std::size_t h);

void tex_convert_rgb888_to_bgr565(std::vector<uint8_t>& output,
                                  const uint8_t* input, std::size_t w,
                                  std::size_t h);

void tex_convert_rgb565_to_bgr565(std::vector<uint8_t>& output,
                                  const uint8_t* input, std::size_t w,
                                  std::size_t h);

void tex_convert_rgba8888_to_abgr8888(std::vector<uint8_t>& output,
                                      const uint8_t* input, std::size_t w,
                                      std::size_t h);
