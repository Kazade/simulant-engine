#include "dreamcast.h"
#include "../texture.h"

namespace smlt {
namespace utils {

smlt::optional<std::vector<uint8_t>> vmu_lcd_image_from_texture(TexturePtr tex, VMUImageGenerationMode mode) {
    if(tex->format() != smlt::TEXTURE_FORMAT_RGBA_4UB_8888) {
        S_ERROR("Attempted to convert a texture that wasn't RGBA8888");
        return smlt::optional<std::vector<uint8_t>>();
    }

    if(tex->width() != 48 || tex->height() != 32) {
        S_ERROR("Attempted to convert a texture that wasn't 48x32");
        return smlt::optional<std::vector<uint8_t>>();
    }

    std::vector<uint8_t> result;
    result.resize(192);

    if(mode == VMU_IMAGE_GENERATION_MODE_ALPHA) {
        const uint8_t* src = &tex->data()[0];
        for(int y = 0; y < 32; ++y) {
            uint64_t row = 0;

            for(int x = 0; x < 48; ++x, src += 4) {
                uint8_t alpha = *(src + 3);
                row |= uint64_t(alpha > 0) << x;
            }

            const uint8_t* byte = (const uint8_t*) &row;

            for(int i = 5; i >= 0; --i) {
                result[(y * 6) + (5 - i)] = *(byte + i);
            }
        }
    } else {
        S_ERROR("Unsupported");
        return smlt::optional<std::vector<uint8_t>>();
    }

    return result;
}

}
}

