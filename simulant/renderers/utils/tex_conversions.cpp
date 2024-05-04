#include "tex_conversions.h"

void tex_convert_argb4444_to_abgr4444(std::vector<uint8_t>& output,
                                      const uint8_t* input, std::size_t w,
                                      std::size_t h) {

    /* FIXME: Do this with 32bit pairs for speed */

    auto texel_count = w * h;
    output.resize(texel_count * 2);

    uint16_t* data16 = (uint16_t*)input;
    for(std::size_t i = 0; i < texel_count; ++i) {
        uint16_t inp = data16[i];
        uint16_t texel = (inp & 0xF000) | ((inp >> 8) & 0xF) | (inp & 0xF0) |
                         (inp & 0xF) << 8;

        uint16_t* target = (uint16_t*)(&output[i * 2]);
        *target = texel;
    }
}

void tex_convert_rgb888_to_bgr565(std::vector<uint8_t>& output,
                                  const uint8_t* input, std::size_t w,
                                  std::size_t h) {
    auto texel_count = w * h;
    output.resize(texel_count * 2);

    for(std::size_t i = 0; i < texel_count; ++i) {
        auto off = (i * 3);
        uint16_t texel = ((input[off + 2] >> 3) << 11) |
                         ((input[off + 1] >> 2) << 5) |
                         ((input[off + 0] >> 3) << 0);

        uint16_t* target = (uint16_t*)(&output[i * 2]);
        *target = texel;
    }
}

void tex_convert_rgb565_to_bgr565(std::vector<uint8_t>& output,
                                  const uint8_t* input, std::size_t w,
                                  std::size_t h) {
    /* FIXME: Do this with 32bit pairs for speed */

    auto texel_count = w * h;
    output.resize(texel_count * 2);

    uint16_t* data16 = (uint16_t*)input;
    for(std::size_t i = 0; i < texel_count; ++i) {
        uint16_t inp = data16[i];
        uint16_t texel =
            ((inp & 0x1F) << 11) | (inp & 0x7E0) | ((inp >> 11) & 0x1F);

        uint16_t* target = (uint16_t*)(&output[i * 2]);
        *target = texel;
    }
}

void tex_convert_rgba8888_to_abgr8888(std::vector<uint8_t>& output,
                                      const uint8_t* input, std::size_t w,
                                      std::size_t h) {
    auto texel_count = w * h;
    output.resize(texel_count * 4);

    for(std::size_t i = 0; i < texel_count; ++i) {
        auto off = (i * 4);

        output[off + 0] = input[off + 3];
        output[off + 1] = input[off + 2];
        output[off + 2] = input[off + 1];
        output[off + 3] = input[off + 0];
    }
}
