#include "psp_texture_manager.h"
#include "../../core/memory.h"
#include "../utils/mipmapping.h"
#include "psp_renderer.h"
#include <map>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspkernel.h>

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

    return final_data_size + (calc_row_stride(format, w) * h);
}

static inline bool is_paletted_format(int format) {
    return format == GU_PSM_T4 || format == GU_PSM_T8;
}

static std::size_t generate_mipmaps(std::vector<uint8_t>& out,
                                    PSPMipmapVector& mipmaps, int format,
                                    std::size_t w, std::size_t h,
                                    const uint8_t*& data) {

    auto row_stride = calc_row_stride(format, w);
    std::size_t data_size = row_stride * h;
    auto final_data_size = calc_mipmap_data_required(format, w, h);

    S_DEBUG("[MIP] Final data size: {0} for {1}x{2}", final_data_size, w, h);

    out.resize(final_data_size);

    uint8_t* new_data = &out[0];

    mipmaps.clear();

    PSPMipmap base;
    base.offset = 0;
    base.w = w;
    base.h = h;

    mipmaps.push_back(base);

    if(is_paletted_format(format)) {
        // FIXME: Support paletted texture mipmaps
        swizzle(new_data, data, row_stride, h);
        return data_size;
    } else {
        S_DEBUG("[MIP] Copied {0} bytes to level 0", data_size);
        std::memcpy(new_data, data, data_size);

        int offset = calc_row_stride(format, w) * h;
        w >>= 1;
        h >>= 1;

        const uint8_t* src = &new_data[0];
        uint8_t* dest = &new_data[0] + offset;

        while(true) {
            PSPMipmap mipmap;
            mipmap.offset = (dest - &new_data[0]);
            mipmap.w = w;
            mipmap.h = h;
            mipmaps.push_back(mipmap);

            if(format == GU_PSM_5650) {
                auto new_level_size =
                    generate_mipmap_level_rgb565(w * 2, h * 2, src, dest);

                src = dest;
                dest += new_level_size;
            } else if(format == GU_PSM_4444) {
                auto new_level_size =
                    generate_mipmap_level_rgba4444(w * 2, h * 2, src, dest);

                src = dest;
                dest += new_level_size;
            } else if(format == GU_PSM_5551) {
                auto new_level_size =
                    generate_mipmap_level_rgba5551(w * 2, h * 2, src, dest);

                src = dest;
                dest += new_level_size;
            } else {
                assert(format == GU_PSM_8888);
                auto new_level_size =
                    generate_mipmap_level_rgba8888(w * 2, h * 2, src, dest);

                src = dest;
                dest += new_level_size;
            }

            if(w == 1 && h == 1) {
                // We're done!
                break;
            }

            w >>= 1;
            h >>= 1;

            w = std::max((int)w, 1);
            h = std::max((int)h, 1);
        }

        /* Now we have to swizzle each mipmap level */
        for(auto& map: mipmaps) {
            auto row_stride = calc_row_stride(format, map.w);
            std::vector<uint8_t> temp(row_stride * map.h);
            swizzle(&temp[0], &new_data[0] + map.offset, row_stride, map.h);
            std::memcpy(&new_data[0] + map.offset, &temp[0], temp.size());
        }

        data = new_data;

        return final_data_size;
    }
}

int PSPTextureManager::upload_texture(int id, int format, int width, int height,
                                      std::size_t data_size,
                                      const uint8_t* data,
                                      const uint8_t* palette,
                                      std::size_t palette_size,
                                      int palette_format, bool do_mipmaps) {

    static int id_counter = 0;
    static std::vector<uint8_t> buffer;
    buffer.clear();

    PSPMipmapVector mipmaps;

    bool is_swizzled = false;

    // FIXME: generate mipmaps for paletted formats
    if(is_paletted_format(format)) {
        do_mipmaps = false;
    }

    /* We don't currently allowed swizzled versions of texture formats, which
     * means users can't just provide their pre-swizzled textures, so we
     * always swizzle here - in future we can change that and avoid the
     * perf cost. */
    if(do_mipmaps) {
        data_size =
            generate_mipmaps(buffer, mipmaps, format, width, height, data);
        data = &buffer[0];
        is_swizzled = true;
    } else {
        int row_stride = calc_row_stride(format, width);
        buffer.resize(data_size);
        swizzle(&buffer[0], data, row_stride, height);
        data = &buffer[0];
        is_swizzled = true;

        // Specify the base level image
        PSPMipmap base;
        base.offset = 0;
        base.w = width;
        base.h = height;
        mipmaps.push_back(base);
    }

    S_DEBUG("Uploading texture data of size: {0} format: {1} pformat: {2}",
            data_size, format, palette_format);

    if(id == 0) {
        PSPTextureObject obj;
        obj.id = ++id_counter;
        obj.format = format;
        obj.width = width;
        obj.height = height;
        obj.data_size = data_size;
        obj.priority = max_priority;
        obj.mipmaps = mipmaps;
        obj.is_swizzled = is_swizzled;

        if(is_paletted_format(obj.format)) {
            obj.palette = (uint8_t*)aligned_alloc(16, palette_size);
            assert(obj.palette);

            std::memcpy(obj.palette, palette, palette_size);
            obj.palette_format = palette_format;
            obj.palette_size = palette_size;
        } else {
            obj.palette = nullptr;
        }

        // FIXME: If this malloc fails, what should we do?
        obj.texture_ram = (uint8_t*)aligned_alloc(16, data_size);
        std::memcpy(obj.texture_ram, data, data_size);

        obj.texture_vram = nullptr;

        textures_.push_back(obj);

        // This will promote to vram if possible
        bind_texture(obj.id);

        S_DEBUG("Created texture: {0}", obj.id);

        return obj.id;
    } else {
        auto obj = find_texture(id);
        if(obj) {
            vram_alloc_free(nullptr, obj->texture_vram);
            obj->texture_vram = 0;
            obj->priority = 0;

            // Free the data that's there
            free(obj->texture_ram);
            obj->texture_ram = nullptr;

            free(obj->palette);
            obj->palette = nullptr;

            obj->width = width;
            obj->height = height;
            obj->data_size = data_size;
            obj->format = format;
            obj->mipmaps = mipmaps;
            obj->is_swizzled = is_swizzled;
            obj->texture_ram = (uint8_t*)aligned_alloc(16, data_size);
            obj->texture_vram = nullptr;

            std::memcpy(obj->texture_ram, data, data_size);

            if(is_paletted_format(obj->format)) {
                obj->palette = (uint8_t*)aligned_alloc(16, palette_size);
                assert(obj->palette);

                std::memcpy(obj->palette, palette, palette_size);
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
    if(currently_bound_texture_ == id) {
        // Don't bind unnecessarily
        return;
    }

    auto tex = find_texture(id);
    if(tex) {
        currently_bound_texture_ = tex->id;

        tex->priority = max_priority; // Reset the texture priority

        auto available = vram_alloc_count_continuous((void*)0);

        if(!tex->texture_vram && (available < tex->data_size ||
                                  lowest_texture_priority_ < tex->priority)) {
            /* We've bound a texture not in vram, and there's a texture with a
             * lower priority. Let's find out what it is! */
            if(space_in_vram(tex)) {
                promote_texture(tex);
            } else {
                S_ERROR("No space in vram. {0} vs {1}", tex->data_size,
                        available);
            }
        } else {
            S_ERROR("Not promoting: {0} vs {1} - {2} vs {3}", available,
                    tex->data_size, lowest_texture_priority_, tex->priority);
        }

        if(tex->palette) {            
            auto entries = tex->palette_size /
                           ((tex->palette_format == GU_PSM_8888) ? 4 : 2);

            sceGuClutMode(tex->palette_format, 0, entries - 1, 0);
            sceGuClutLoad(entries / 8, tex->palette);
        }

        assert(tex->mipmaps.size() > 0);
        sceGuTexMode(tex->format, std::min(tex->mipmaps.size(), 8u), 0,
                     (tex->is_swizzled) ? GU_TRUE : GU_FALSE);

        auto data = tex->texture_vram ? tex->texture_vram : tex->texture_ram;
        int i = 0;

        for(auto& level: tex->mipmaps) {
            if(i > 7) {
                // PSP only supports up to 8 levels
                break;
            }

            sceGuTexImage(i++, level.w, level.h, level.w, data + level.offset);
        }

        if(tex->mipmaps.size() > 1) {
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
    auto pool_size = vram_alloc_pool_size((void*)0);
    if(obj->data_size >= pool_size) {
        // No amount of evicting will help us
        S_DEBUG("Not enough room in pool to store texture in vram: {0} vs {1}",
                obj->data_size, pool_size);
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
