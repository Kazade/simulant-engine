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

PSPTextureManager::PSPTextureManager(PSPRenderer* renderer) :
    renderer_(renderer) {}

PSPTextureManager::~PSPTextureManager() {
    auto textures = textures_;
    for(auto& tex: textures) {
        release_texture(tex.id);
    }
}

int PSPTextureManager::upload_texture(int id, int format, int width, int height,
                                      std::size_t data_size,
                                      const uint8_t* data,
                                      const uint8_t* palette,
                                      std::size_t palette_size,
                                      int palette_format) {

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

            sceGuClutMode(tex->palette_format, 0, 0, 0);
            sceGuClutLoad(entries / 8, tex->palette);
        }

        // FIXME: mipmap
        // FIXME: Swizzle
        sceGuTexMode(tex->format, 0, 0, GU_FALSE);

        // FIXME: mipmap
        sceGuTexImage(0, tex->width, tex->height, tex->width,
                      tex->texture_vram ? tex->texture_vram : tex->texture_ram);

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
