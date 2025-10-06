
#include "png_loader.h"

#include "pngle/pngle.h"

namespace smlt {
namespace loaders {

struct UserData {
    Texture* texture;
    bool initialized;
};

static void draw_cb(pngle_t* pngle, uint32_t x, uint32_t y, uint32_t w,
                    uint32_t h, const uint8_t rgba[4]) {

    UserData* user_data = (UserData*)pngle_get_user_data(pngle);
    auto result = user_data->texture;

    uint8_t* data = nullptr;
    if(!user_data->initialized) {
        user_data->initialized = true;
        // We need to set the format before we do anything
        auto hdr = pngle_get_ihdr(pngle);

        // color type 2 is true color (no alpha) and color type 3 is paletted
        // which is 24 bit. For both we use RGB.
        auto channels = (hdr->color_type == 2 || hdr->color_type == 3) ? 3 : 4;
        // fixme: we can convert this down based on the header color type
        // to save on ram/vram
        auto format = (channels == 3) ? TEXTURE_FORMAT_RGB_3UB_888
                                      : TEXTURE_FORMAT_RGBA_4UB_8888;
        result->set_format(format);
        result->resize(hdr->width, hdr->height);
    }

    data = result->map_data(result->data_size());

    auto idx = (y * result->width()) + x;
    for(std::size_t c = 0; c < result->channels(); ++c) {
        data[(idx * result->channels()) + c] = rgba[c];
    }
}

bool PNGLoader::do_load(std::shared_ptr<FileIfstream> stream, Texture* result) {

    UserData data;
    data.texture = result;
    data.initialized = false;

    auto pngl = pngle_new();
    pngle_set_user_data(pngl, &data);
    pngle_set_draw_callback(pngl, draw_cb);

    const uint32_t chunk_size = 32 * 1024;
    char buffer[chunk_size];
    while(stream->good()) {
        stream->read(buffer, chunk_size);
        auto bytes = stream->gcount();
        auto ret = pngle_feed(pngl, (const void*)buffer, bytes);
        if(ret == -1) {
            S_ERROR("Failed to read PNG");
            return result;
        }
    }

    pngle_destroy(pngl);

    return result;
}

} // namespace loaders
} // namespace smlt
