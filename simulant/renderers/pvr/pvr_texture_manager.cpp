#include "pvr_texture_manager.h"
#include "../../core/memory.h"
#include "../../generic/raii.h"
#include "../utils/mipmapping.h"

#include "pvr_api.h"
#include "pvr_renderer.h"

#include <map>

namespace smlt {

void update_data_pointer(void* src, void* dst, void* data) {
    PVRTextureManager* self = (PVRTextureManager*)data;

    for(auto& texture: self->textures_) {
        if(texture->texture_vram == src) {
            texture->texture_vram = (uint8_t*)dst;
            return;
        }
    }
}

static std::size_t calc_row_stride(pvr_tex_format_t format, std::size_t width) {
    switch(format) {
        case PVR_TEX_FORMAT_PAL_4BPP:
            return width / 2;
        case PVR_TEX_FORMAT_PAL_8BPP:
            return width;
        default:
            return width * 2;
    }
}

PVRTextureManager::PVRTextureManager(PVRRenderer* renderer) :
    renderer_(renderer) {}

PVRTextureManager::~PVRTextureManager() {
    auto textures = textures_;
    for(auto& tex: textures) {
        release_texture(tex->id);
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

#define ALIGN16(x) ((x + (15)) & ~15)

static std::size_t calc_mipmap_data_required(pvr_tex_format_t format, std::size_t w,
                                             std::size_t h) {
    std::size_t final_data_size = 0;
    while(w > 1 || h > 1) {
        final_data_size += calc_row_stride(format, w) * h;
        final_data_size = ALIGN16(final_data_size);

        if(w > 1) {
            w /= 2;
        }
        if(h > 1) {
            h /= 2;
        }
    }

    return ALIGN16(final_data_size + ALIGN16(calc_row_stride(format, w) * h));
}

static inline bool is_paletted_format(int format) {
    return format == PVR_TEX_FORMAT_PAL_4BPP || format == PVR_TEX_FORMAT_PAL_8BPP;
}

static std::size_t generate_mipmaps(std::vector<uint8_t>& out,
                                    PVRMipmapVector& mipmaps, pvr_tex_format_t format,
                                    std::size_t w, std::size_t h,
                                    const uint8_t*& data) {

    auto row_stride = calc_row_stride(format, w);
    std::size_t data_size = row_stride * h;
    auto final_data_size = calc_mipmap_data_required(format, w, h);

    S_VERBOSE("[MIP] Final data size: {0} for {1}x{2}", final_data_size, w, h);

    out.resize(final_data_size);

    uint8_t* new_data = &out[0];

    mipmaps.clear();

    PVRMipmap base;
    base.offset = 0;
    base.w = w;
    base.h = h;

    mipmaps.push_back(base);

    if(is_paletted_format(format)) {
        // FIXME: Support paletted texture mipmaps
        swizzle(new_data, data, row_stride, h);
        return data_size;
    } else {
        S_VERBOSE("[MIP] Copied {0} bytes to level 0", data_size);
        std::memcpy(new_data, data, data_size);

        int offset = ALIGN16(calc_row_stride(format, w) * h);
        w >>= 1;
        h >>= 1;

        const uint8_t* src = &new_data[0];
        uint8_t* dest = &new_data[0] + offset;

        while(true) {
            PVRMipmap mipmap;
            mipmap.offset = (dest - &new_data[0]);
            mipmap.w = w;
            mipmap.h = h;
            mipmaps.push_back(mipmap);

            if(format == PVR_TEX_FORMAT_RGB565) {
                auto new_level_size =
                    generate_mipmap_level_rgb565(w * 2, h * 2, src, dest);

                src = dest;
                dest += ALIGN16(new_level_size);
            } else if(format == PVR_TEX_FORMAT_RGBA4444) {
                auto new_level_size =
                    generate_mipmap_level_rgba4444(w * 2, h * 2, src, dest);

                src = dest;
                dest += ALIGN16(new_level_size);
            } else if(format == PVR_TEX_FORMAT_RGBA5551) {
                auto new_level_size =
                    generate_mipmap_level_rgba5551(w * 2, h * 2, src, dest);

                src = dest;
                dest += ALIGN16(new_level_size);
            } else {
                S_ERROR("Unsupported format for mipmap generation: {0}",
                        format);
                break;
            }

            if(w == 1 && h == 1) {
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

int PVRTextureManager::upload_texture(int id, pvr_tex_format_t format, int width, int height, std::size_t data_size,
                                      const uint8_t* data, const uint8_t* palette, std::size_t palette_size,
                                      pvr_palette_format_t palette_format, bool do_mipmaps) {

    S_DEBUG("upload_texture");

    static int id_counter = 0;
    static std::vector<uint8_t> buffer;
    buffer.clear();

    // Make sure we always clear the buffer when we're done with it
    smlt::raii::Finally finally([&]() {
        buffer.clear();
    });

    if(width > 512 || height > 512) {
        S_ERROR("Tried to upload a texture beyond the max texture size");
        return 0;
    }

    PVRMipmapVector mipmaps;

    bool is_twiddled = false;

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
        is_twiddled = true;
    } else {
        int row_stride = calc_row_stride(format, width);
        buffer.resize(data_size);
        swizzle(&buffer[0], data, row_stride, height);
        data = &buffer[0];
        is_twiddled = true;

        // Specify the base level image
        PVRMipmap base;
        base.offset = 0;
        base.w = width;
        base.h = height;
        mipmaps.push_back(base);
    }

    S_DEBUG("Uploading texture data of size: {0} format: {1} pformat: {2}",
            data_size, format, palette_format);

    bool created = id == 0;
    std::shared_ptr<PVRTextureObject> obj;
    if(id != 0) {
        obj = find_texture(id);
        if(!obj) {
            S_ERROR("Tried to update invalid texture");
            return 0;
        }

    } else {
        obj = std::make_shared<PVRTextureObject>();
        obj->id = ++id_counter;
    }

    vram_alloc_free(nullptr, obj->texture_vram);
    obj->texture_vram = 0;

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
    obj->is_twiddled = is_twiddled;
    obj->texture_ram = (uint8_t*)aligned_alloc(16, data_size);
    obj->texture_vram = nullptr;

    if(!obj->texture_ram) {
        S_ERROR("Failed to allocate memory ({0} bytes) for texture", data_size);
        release_texture(obj->id);
        return 0;
    }

    obj->can_fit_in_vram = data_size < vram_alloc_pool_size(nullptr) &&
                           (obj->width < 512 && obj->height < 512);

    std::memcpy(obj->texture_ram, data, data_size);

    if(is_paletted_format(obj->format)) {
        obj->palette = (uint8_t*)aligned_alloc(16, palette_size);
        assert(obj->palette);

        std::memcpy(obj->palette, palette, palette_size);
        obj->palette_format = palette_format;
        obj->palette_size = palette_size;
    } else {
        obj->palette = nullptr;
        obj->palette_format = PVR_PALETTE_FORMAT_NONE;
        obj->palette_size = 0;
    }

    textures_.push_back(obj);
    texture_priority_.push_front(obj.get());

    if(created) {
        S_DEBUG("Created texture: {0}", obj->id);
    } else {
        S_VERBOSE("Updated texture: {0}", obj->id);
    }

    return obj->id;
}

bool PVRTextureManager::promote_texture(PVRTextureObject* obj) {
    if(obj && !obj->texture_vram && obj->texture_ram) {
        S_VERBOSE("Promoting {0} (size: {1}) to VRAM", obj->id, obj->data_size);
        auto size = obj->data_size;
        // Move into vram
        obj->texture_vram = (uint8_t*)vram_alloc_malloc((void*)0, size);

        if(!obj->texture_vram) {
            S_VERBOSE("Couldn't allocate vram. {0} vs {1}", size,
                      vram_alloc_count_continuous(nullptr));
            return false;
        }

        S_DEBUG("Copying data from {0} to {1}", (intptr_t)obj->texture_ram,
                (intptr_t)obj->texture_vram);

        S_DEBUG("Allocated {0} bytes ({1}). Remaining: {2}\n", size,
                (intptr_t)obj->texture_vram, vram_alloc_count_free(nullptr));

        std::memcpy(obj->texture_vram, obj->texture_ram, obj->data_size);

        free(obj->texture_ram);
        obj->texture_ram = nullptr;
    }

    return true;
}

void PVRTextureManager::bind_texture(int id) {

    if(currently_bound_texture_ == id) {
        // Don't bind unnecessarily
        return;
    }

    auto tex = find_texture(id);
    if(tex) {
        currently_bound_texture_ = tex->id;

        if(!tex->texture_ram && !tex->texture_vram) {
            // Can't do anything with a texture that has no data
            S_ERROR("Texture ({0}) had no data and couldn't be bound", id);
            return;
        }

        textures_this_frame_.insert(tex.get());
        texture_priority_.erase(std::find(texture_priority_.begin(),
                                          texture_priority_.end(), tex.get()));
        texture_priority_.push_front(tex.get());

        if(tex->palette) {
            auto entries =
                tex->palette_size /
                ((tex->palette_format == PVR_PALETTE_FORMAT_RGBA8888) ? 4 : 2);

            pvr_clut_mode(tex->palette_format);
            pvr_clut_load(entries, tex->palette);
        }

        assert(tex->mipmaps.size() > 0);

        auto levels = std::max(std::min(tex->mipmaps.size(), (std::size_t) 8u), (std::size_t) 1u);

        pvr_tex_mode(tex->format, levels - 1,
                     (tex->is_twiddled) ? PVR_TRUE : PVR_FALSE);

        auto data = tex->texture_vram ? tex->texture_vram : tex->texture_ram;
        if(!data) {
            S_ERROR("Texture ({0}) had no data after promotion and couldn't be "
                    "bound",
                    id);
            return;
        }

        for(std::size_t i = 0; i < levels; ++i) {
            auto& level = tex->mipmaps[i];
            auto dest = data + level.offset;
            pvr_tex_image(i, level.w, level.h, dest);
        }

        if(levels > 1) {
            switch(tex->filter) {
                case TEXTURE_FILTER_BILINEAR:
                    pvr_tex_filter(PVR_TEX_FILTER_LINEAR,
                                   PVR_TEX_FILTER_LINEAR_MIPMAP_NEAREST);
                    break;
                case TEXTURE_FILTER_TRILINEAR:
                    pvr_tex_filter(PVR_TEX_FILTER_LINEAR,
                                   PVR_TEX_FILTER_LINEAR_MIPMAP_LINEAR);
                    break;
                default:
                    pvr_tex_filter(PVR_TEX_FILTER_NEAREST,
                                   PVR_TEX_FILTER_NEAREST);
                    break;
            }
        } else {
            switch(tex->filter) {
                case TEXTURE_FILTER_BILINEAR:
                case TEXTURE_FILTER_TRILINEAR:
                    pvr_tex_filter(PVR_TEX_FILTER_LINEAR,
                                   PVR_TEX_FILTER_LINEAR);
                    break;
                default:
                    pvr_tex_filter(PVR_TEX_FILTER_NEAREST,
                                   PVR_TEX_FILTER_NEAREST);
                    break;
            }
        }
    } else {
        currently_bound_texture_ = 0;
    }
}

void PVRTextureManager::release_texture(int id) {
    auto obj = find_texture(id);

    S_INFO("Releasing texture: {0}", id);

    assert(obj);

    if(!obj) {
        S_DEBUG("Tried to release non existant texture: {0}", id);
        return;
    }

    if(obj->texture_vram) {
        vram_alloc_free((void*)0, obj->texture_vram);
        obj->texture_vram = nullptr;
    }

    if(obj->texture_ram) {
        free(obj->texture_ram);
        obj->texture_ram = nullptr;
    }

    free(obj->palette);
    obj->palette = nullptr;
    obj->id = 0;
    textures_this_frame_.erase(obj.get());
    texture_priority_.erase(std::find(texture_priority_.begin(),
                                      texture_priority_.end(), obj.get()));
    textures_.erase(std::remove(textures_.begin(), textures_.end(), obj));
}

void PVRTextureManager::update_priorities() {
    int space_required = 0;

    static std::vector<PVRTextureObject*> to_promote;
    to_promote.clear();

    for(auto tex: textures_this_frame_) {
        if(tex->texture_vram) {
            continue;
        }

        space_required += tex->data_size;
        to_promote.push_back(tex);
    }

    int available = (int)vram_alloc_count_free(nullptr);

    /* Under-estimate the available memory, in case defragging doesn't
     * give it all to us */
    available *= 0.8;

    space_required = std::max(space_required - available, 0);

    if(space_required > 0) {
        for(auto it = texture_priority_.rbegin();
            it != texture_priority_.rend(); ++it) {

            auto t = *it;
            if(t->texture_vram && !textures_this_frame_.count(*it)) {
                evict_texture(t);
                space_required -= t->data_size;

                if(space_required <= 0) {
                    break;
                }
            }
        }

        // Run a defrag to make sure we maximise the free space
        vram_alloc_run_defrag((void*)0, update_data_pointer, 5, this);
    }

    textures_this_frame_.clear();

    for(auto& t: to_promote) {
        if(!promote_texture(t)) {
            S_VERBOSE("Couldn't promote texture {0}", t->id);
        }
    }
}

bool PVRTextureManager::evict_texture(PVRTextureObject* obj) {
    // If we found a texture, move its data out of vram
    if(obj && obj->texture_vram) {
        auto size = obj->data_size;
        // Move out of vram
        obj->texture_ram = (uint8_t*)aligned_alloc(16, size);
        if(!obj->texture_ram) {
            S_ERROR("Couldn't allocate enough ram to demote texture {0}",
                    obj->id);
            return false;
        }

        std::memcpy(obj->texture_ram, obj->texture_vram, size);

        vram_alloc_free(nullptr, obj->texture_vram);
        obj->texture_vram = nullptr;

        S_VERBOSE("Demoted {0} (size: {1}) to RAM", obj->id, obj->data_size);
    }

    return true;
}

std::shared_ptr<PVRTextureObject> PVRTextureManager::find_texture(int id) {
    for(auto& tex: textures_) {
        if(tex->id == id) {
            return tex;
        }
    }

    return nullptr;
}

} // namespace smlt
