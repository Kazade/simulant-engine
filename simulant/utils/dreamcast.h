/*
 * These are utility functions that are only really useful for the Dreamcast platform
 */

#pragma once

#include <vector>
#include <cstdint>
#include "../types.h"

namespace smlt {
namespace utils {

enum VMUImageGenerationMode {
    VMU_IMAGE_GENERATION_MODE_ALPHA,
    VMU_IMAGE_GENERATION_MODE_COLOUR
};

/**
 * @brief vmu_lcd_image_from_texture
 * Given a texture, this generates a 48x32 1-bit image with the following caveats:
 *  - The input must be the correct size
 *  - All pixels with an alpha of zero, will be zero. All other pixels will be 1.
 *  - The result is 256 bytes as each line must be padded to 8 bytes (64 bits)
 * @param tex
 * @return packed texture data
 */
smlt::optional<std::vector<uint8_t> > vmu_lcd_image_from_texture(TexturePtr tex, VMUImageGenerationMode mode);

}
}
