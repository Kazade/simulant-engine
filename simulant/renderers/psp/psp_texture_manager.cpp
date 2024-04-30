#include "../../core/memory.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

#include "psp_texture_manager.h"
#include "psp_renderer.h"

namespace smlt {

const static int max_priority =
    std::numeric_limits<decltype(PSPTextureObject::priority)>::max();

void update_data_pointer(void* src, void* dst, void* data) {

    PSPTextureManager* self = (PSPTextureManager*)data;

    for(auto& texture: self->textures_) {
        if(texture.texture_vram == src) {
            texture.texture_vram = (uint8_t*)dst;
            return;
        }
    }
}

static std::size_t calc_row_stride(int format, std::size_t width) {
    switch(format) {
        case GU_PSM_8888:
            return width * 4;
        case GU_PSM_T4:
            return width / 2;
        case GU_PSM_T8:
            return width;
        default:
            return width * 2;
    }
}

PSPTextureManager::PSPTextureManager(PSPRenderer* renderer) :
    renderer_(renderer) {}

PSPTextureManager::~PSPTextureManager() {
    auto textures = textures_;
    for(auto& tex: textures) {
        release_texture(tex.id);
    }
}

static void swizzle(uint8_t* out, const uint8_t* in, std::size_t width,
                    std::size_t height) {

    std::size_t width_blocks = (width / 16);
    std::size_t height_blocks = (height / 8);

    std::size_t src_pitch = (width - 16) / 4;
    std::size_t src_row = width * 8;

    const uint8_t* ysrc = in;
    uint32_t* dst = (uint32_t*)out;

    for(std::size_t blocky = 0; blocky < height_blocks; ++blocky) {
        const uint8_t* xsrc = ysrc;
        for(std::size_t blockx = 0; blockx < width_blocks; ++blockx) {
            const uint32_t* src = (uint32_t*)xsrc;
            for(std::size_t j = 0; j < 8; ++j) {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                src += src_pitch;
            }
            xsrc += 16;
        }
        ysrc += src_row;
    }
}

static std::size_t calc_mipmap_data_required(int format, std::size_t w,
                                             std::size_t h) {
    std::size_t final_data_size = 0;
    while(w > 1 || h > 1) {
        final_data_size += calc_row_stride(format, w) * h;
        if(w > 1) {
            w /= 2;
        }
        if(h > 1) {
            h /= 2;
        }
    }

    return final_data_size;
}

static std::size_t generate_mipmaps(std::vector<uint8_t>& out, int format,
                                    std::size_t w, std::size_t h,
                                    const uint8_t*& data) {

    auto row_stride = calc_row_stride(format, w);
    std::size_t data_size = row_stride * h;
    auto final_data_size = calc_mipmap_data_required(format, w, h);

    S_DEBUG("[MIP] Final data size: {0} for {1}x{2}", final_data_size, w, h);

    out.resize(final_data_size);

    uint8_t* new_data = &out[0];

    if(format == GU_PSM_T4 || format == GU_PSM_T8) {
        // FIXME: Support paletted texture mipmaps
        swizzle(new_data, data, row_stride, h);
        return data_size;
    } else {
        S_DEBUG("[MIP] Copied {0} bytes to level 0", data_size);
        std::memcpy(new_data, data, data_size);
    }

    uint8_t* src = new_data;
    uint8_t* dest = new_data + data_size;

    int texel_stride = (format == GU_PSM_8888) ? 4 : 2;

    struct Mipmap {
        uint8_t* ptr;
        std::size_t row_stride;
        std::size_t h;
    };

    std::vector<Mipmap> mipmaps;

    Mipmap base;
    base.ptr = src;
    base.row_stride = row_stride;
    base.h = h;

    mipmaps.push_back(base);

    while(w > 1 || h > 1) {
        auto lw = w;
        auto lh = h;
        auto source_row_stride = calc_row_stride(format, w);
        auto dest_row_stride = calc_row_stride(format, w / 2);

        S_DEBUG("[MIP] SRS: {0}, DRS: {1}", source_row_stride, dest_row_stride);

        if(w > 1) {
            w /= 2;
        }

        if(h > 1) {
            h /= 2;
        }

        Mipmap map;
        map.ptr = dest;
        map.row_stride = dest_row_stride;
        map.h = h;

        mipmaps.push_back(map);

        for(std::size_t y = 0; y < lh; y += 2) {
            for(std::size_t x = 0; x < lw; x += 2) {

                int source_idx = ((y * source_row_stride) + x);
                int dest_idx = (((y / 2) * dest_row_stride) + (x / 2));

                if(format == GU_PSM_8888) {

                } else {
                    uint16_t* src_t = (uint16_t*)(src + source_idx);
                    uint16_t* dest_t = (uint16_t*)(dest + dest_idx);

                    uint16_t t0 = *src_t;
                    uint16_t t1 = *(src_t + 1);
                    uint16_t t2 = *(src_t + source_row_stride);
                    uint16_t t3 = *(src_t + source_row_stride + 1);

                    uint16_t final = 0;
                    if(format == GU_PSM_5650) {
                        int r = 0, g = 0, b = 0;
                        for(const uint16_t& t: {t0, t1, t2, t3}) {
                            r += (t >> 11) & 0x1F;
                            g += ((t >> 5) & 0x3F);
                            b += (t & 0x1F);
                        }

                        r /= 4;
                        g /= 4;
                        b /= 4;

                        final = (r << 11) | (g << 5) | (b);
                    } else if(format == GU_PSM_4444) {
                        int r = 0, g = 0, b = 0, a = 0;

                        for(const uint16_t& t: {t0, t1, t2, t3}) {
                            r += (t >> 12);
                            g += ((t >> 8) & 0xF);
                            b += ((t >> 4) & 0xF);
                            a += (t & 0xF);
                        }

                        r /= 4;
                        g /= 4;
                        b /= 4;
                        a /= 4;

                        final = (r << 12) | (g << 8) | (b << 4) | (a);
                    } else if(format == GU_PSM_5551) {
                        int r = 0, g = 0, b = 0, a = 0;

                        for(const uint16_t& t: {t0, t1, t2, t3}) {
                            r += ((t >> 11) & 0x1F);
                            g += ((t >> 6) & 0x1F);
                            b += ((t >> 1) & 0x1F);
                            a += (t & 0x1);
                        }

                        r /= 4;
                        g /= 4;
                        b /= 4;
                        a /= 4;

                        final = (r << 11) | (g << 6) | (b << 1) | (a & 0x1);
                    }

                    *dest_t = final;
                }
            }
        }

        src = dest;
        dest += calc_row_stride(format, w) * h;
    }

    /* Now we have to swizzle each mipmap level */
    for(auto& map: mipmaps) {
        std::vector<uint8_t> temp(map.row_stride * map.h);
        swizzle(&temp[0], map.ptr, map.row_stride, map.h);
        std::memcpy(map.ptr, &temp[0], temp.size());
    }

    data = new_data;

    return final_data_size;
}

int PSPTextureManager::upload_texture(int id, int format, int width, int height,
                                      std::size_t data_size,
                                      const uint8_t* data,
                                      const uint8_t* palette,
                                      std::size_t palette_size,
                                      int palette_format, bool do_mipmaps) {

    std::vector<uint8_t> buffer;

    /* We don't currently allowed swizzled versions of texture formats, which
     * means users can't just provide their pre-swizzled textures, so we
     * always swizzle here - in future we can change that and avoid the
     * perf cost. We don't currently swizzled paletted textures. */

    if(do_mipmaps) {
        data_size = generate_mipmaps(buffer, format, width, height, data);
        data = &buffer[0];
    } else {
        int row_stride = calc_row_stride(format, width);
        buffer.resize(data_size);
        swizzle(&buffer[0], data, row_stride, height);
        data = &buffer[0];
    }

    static int id_counter = 0;

    if(id == 0) {
        PSPTextureObject obj;
        obj.id = ++id_counter;
        obj.format = format;
        obj.width = width;
        obj.height = height;
        obj.data_size = data_size;
        obj.priority = max_priority;

        if(palette && (format == GU_PSM_T4 || format == GU_PSM_T8)) {
            S_DEBUG("Uploading palette data of size {0} format: {1}, "
                    "palette_format: {2}",
                    palette_size, format, palette_format);

            obj.palette = (uint8_t*)aligned_alloc(16, palette_size);
            assert(obj.palette);

            std::memcpy(obj.palette, palette, obj.palette_size);
            obj.palette_format = palette_format;
            obj.palette_size = palette_size;
        } else {
            obj.palette = nullptr;
        }

        // FIXME: If this malloc fails, what should we do?
        obj.texture_ram = (uint8_t*)aligned_alloc(16, data_size);
        std::memcpy(obj.texture_ram, data, data_size);

        // This will promote to vram if possible
        bind_texture(obj.id);

        S_DEBUG("Created texture: {0}", obj.id);

        textures_.push_back(obj);

        return obj.id;
    } else {
        auto obj = find_texture(id);
        if(obj) {
            // Move out of vram if it's there
            evict_texture(obj);

            // Free the data that's there
            free(obj->texture_ram);
            obj->texture_ram = nullptr;

            free(obj->palette);
            obj->palette = nullptr;

            obj->width = width;
            obj->height = height;
            obj->data_size = data_size;
            obj->format = format;
            obj->texture_ram = (uint8_t*)aligned_alloc(16, data_size);
            std::memcpy(obj->texture_ram, data, data_size);

            if(palette && (format == GU_PSM_T4 || format == GU_PSM_T8)) {
                S_DEBUG("Uploading palette data of size {0} format: {1}, "
                        "palette_format: {2}",
                        palette_size, format, palette_format);
                obj->palette = (uint8_t*)aligned_alloc(16, palette_size);
                assert(obj->palette);

                std::memcpy(obj->palette, palette, obj->palette_size);
                obj->palette_format = palette_format;
                obj->palette_size = palette_size;
            } else {
                obj->palette = nullptr;
                obj->palette_format = 0;
                obj->palette_size = 0;
            }

            bind_texture(obj->id);

            S_DEBUG("Updated texture: {0}", obj->id);
            return obj->id;
        }
    }

    return 0;
}

void PSPTextureManager::promote_texture(PSPTextureObject* obj) {
    if(obj && !obj->texture_vram) {
        S_INFO("Promoted {0} (priority: {1}, size: {2}) to VRAM", obj->id,
               obj->priority, obj->data_size);
        auto size = obj->data_size;
        // Move out of vram
        obj->texture_vram = (uint8_t*)vram_alloc_malloc((void*)0, size);
        std::memcpy(obj->texture_vram, obj->texture_ram, size);

        free(obj->texture_ram);
        obj->texture_ram = nullptr;
        obj->priority = max_priority;
    }
}

void PSPTextureManager::bind_texture(int id) {
    auto tex = find_texture(id);
    if(tex) {
        tex->priority = max_priority; // Reset the texture priority

        if(!tex->texture_vram && lowest_texture_priority_ < tex->priority) {
            /* We've bound a texture not in vram, and there's a texture with a
             * lower priority. Let's find out what it is! */
            if(space_in_vram(tex)) {
                promote_texture(tex);
            }
        }

        if(tex->palette) {
            auto entries = tex->palette_size /
                           ((tex->palette_format == GU_PSM_8888) ? 4 : 2);

            sceGuClutMode(tex->palette_format, 0, 0xFF, 0);
            sceGuClutLoad(entries / 8, tex->palette);
        }

        // FIXME: mipmap
        sceGuTexMode(tex->format, 8, 0, GU_TRUE);

        // FIXME: mipmap
        auto data = tex->texture_vram ? tex->texture_vram : tex->texture_ram;
        sceGuTexImage(0, tex->width, tex->height, tex->width, data);

        if(tex->has_mipmaps) {
            // Specify the other mip levels
            int w = tex->width;
            int h = tex->height;
            uint8_t* mip = data + calc_row_stride(tex->format, w) * h;

            int i = 1;
            w /= 2;
            h /= 2;
            while(w > 1 || h > 1) {
                sceGuTexImage(i++, w, h, w, mip);

                mip += calc_row_stride(tex->format, w) * h;

                if(w > 1) {
                    w /= 2;
                }

                if(h > 1) {
                    h /= 2;
                }
            }
        }

        if(tex->has_mipmaps) {
            switch(tex->filter) {
                case TEXTURE_FILTER_BILINEAR:
                    sceGuTexFilter(GU_LINEAR, GU_LINEAR_MIPMAP_NEAREST);
                    break;
                case TEXTURE_FILTER_TRILINEAR:
                    sceGuTexFilter(GU_LINEAR, GU_LINEAR_MIPMAP_LINEAR);
                    break;
                default:
                    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
                    break;
            }
        } else {
            switch(tex->filter) {
                case TEXTURE_FILTER_BILINEAR:
                case TEXTURE_FILTER_TRILINEAR:
                    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
                    break;
                default:
                    sceGuTexFilter(GU_NEAREST, GU_NEAREST);
                    break;
            }
        }
    }
}

void PSPTextureManager::release_texture(int id) {
    auto obj = find_texture(id);

    assert(obj);

    if(!obj) {
        S_ERROR("Tried to release non existant texture: {0}", id);
        return;
    }

    if(obj->texture_vram) {
        vram_alloc_free((void*)0, obj->texture_vram);
        obj->texture_vram = nullptr;
    } else {
        assert(obj->texture_ram);
        free(obj->texture_ram);
        obj->texture_ram = nullptr;
    }

    free(obj->palette);

    textures_.erase(std::remove_if(textures_.begin(), textures_.end(),
                                   [=](const PSPTextureObject& it) -> bool {
        return &it == obj;
    }));
}

void PSPTextureManager::update_priorities() {
    int lowest = 100000;
    for(auto& tex: textures_) {

        // Avoid wrapping the texture count
        if(tex.priority > 0) {
            tex.priority -= 1;
        }

        if(tex.priority < lowest) {
            lowest = tex.priority;
        }
    }

    lowest_texture_priority_ = lowest;
}

void PSPTextureManager::evict_texture(PSPTextureObject* obj) {

    // If we found a texture, move its data out of vram
    if(obj && obj->texture_vram) {
        S_INFO("Demoted {0} (priority: {1}, size: {2}) to VRAM", obj->id,
               obj->priority, obj->data_size);

        auto size = obj->data_size;
        // Move out of vram
        obj->texture_ram = (uint8_t*)aligned_alloc(16, size);

        std::memcpy(obj->texture_ram, obj->texture_vram, size);

        vram_alloc_free(nullptr, obj->texture_vram);
        obj->texture_vram = nullptr;

        // We set the priority to the lowest, if we get bound again
        // we'll get moved back in if possible
        obj->priority = 0;
    }
}

PSPTextureObject* PSPTextureManager::find_texture(int id) {
    for(auto& tex: textures_) {
        if(tex.id == id) {
            return &tex;
        }
    }

    return nullptr;
}

bool PSPTextureManager::space_in_vram(PSPTextureObject* obj) {

    if(obj->data_size >= vram_alloc_pool_size((void*)0)) {
        // No amount of evicting will help us
        return false;
    }

    auto available = vram_alloc_count_continuous((void*)0);

    if(available >= obj->data_size) {
        return true;
    } else {
        // If there wasn't enough space, run a defrag an see if there is now
        vram_alloc_run_defrag((void*)0, update_data_pointer, 5, this);
        available = vram_alloc_count_continuous((void*)0);
        if(available >= obj->data_size) {
            return true;
        }
    }

    std::map<int, PSPTextureObject*> sorted;

    /* Build a list of textures with a lower priority, that are using vram.
     * These are ordered from lowest priority to highest */
    int possible_to_free = 0;
    for(auto& tex: textures_) {
        if(tex.id == obj->id || !tex.texture_vram ||
           tex.priority >= obj->priority) {
            continue;
        }

        possible_to_free += tex.data_size;
        sorted.insert(std::make_pair(tex.priority, &tex));
    }

    // Assuming we freed everything with a lower priority and then
    // defragged, are we likely to hit the amount we need?
    auto potential = possible_to_free + available;
    if(potential < obj->data_size) {
        return false;
    }

    // Now loop through, free each texture and then defrag until we have
    // enough space
    for(auto& p: sorted) {
        evict_texture(p.second);
        vram_alloc_run_defrag((void*)0, update_data_pointer, 5, this);
        available = vram_alloc_count_continuous((void*)0);
        if(available >= obj->data_size) {
            return true;
        }
    }

    return false;
}

} // namespace smlt
