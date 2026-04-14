#include "pvr_texture_manager.h"
#include "pvr_renderer.h"
#include "../../logging.h"

#include <cstring>
#include <cstdlib>
#include <algorithm>

#ifdef __DREAMCAST__
#include <kos.h>
#include <dc/pvr.h>
#endif

namespace smlt {

PVRTextureManager::PVRTextureManager(PVRRenderer* renderer):
    renderer_(renderer) {
}

PVRTextureManager::~PVRTextureManager() {
    for(auto& tex : textures_) {
        if(tex) {
#ifdef __DREAMCAST__
            if(tex->texture_vram) {
                pvr_mem_free((pvr_ptr_t)tex->texture_vram);
            }
#endif
            if(tex->texture_ram) {
                free(tex->texture_ram);
            }
        }
    }
    textures_.clear();
}

std::shared_ptr<PVRTextureObject> PVRTextureManager::find_texture(int id) {
    for(auto& tex : textures_) {
        if(tex && tex->id == id) {
            return tex;
        }
    }
    return nullptr;
}

int PVRTextureManager::upload_texture(int id, int format, int width, int height,
                                       std::size_t data_size, const uint8_t* data,
                                       bool is_compressed, bool is_twiddled,
                                       bool has_mipmaps, TextureFilter filter) {
    auto existing = find_texture(id);

    if(existing) {
        /* Free old data */
#ifdef __DREAMCAST__
        if(existing->texture_vram) {
            pvr_mem_free((pvr_ptr_t)existing->texture_vram);
            existing->texture_vram = nullptr;
            existing->in_vram = false;
        }
#endif
        if(existing->texture_ram) {
            free(existing->texture_ram);
            existing->texture_ram = nullptr;
        }
    } else {
        existing = std::make_shared<PVRTextureObject>();
        existing->id = id;
        textures_.push_back(existing);
    }

    existing->format = format;
    existing->width = width;
    existing->height = height;
    existing->data_size = data_size;
    existing->is_compressed = is_compressed;
    existing->is_twiddled = is_twiddled;
    existing->has_mipmaps = has_mipmaps;
    existing->filter = filter;

#ifdef __DREAMCAST__
    /* Try to allocate directly in VRAM */
    pvr_ptr_t vram = pvr_mem_malloc(data_size);
    if(vram) {
        /* Load texture data into VRAM via store queues for speed */
        pvr_txr_load(data, vram, data_size);
        existing->texture_vram = vram;
        existing->in_vram = true;
        S_DEBUG("Texture {0} ({1}x{2}) uploaded to VRAM ({3} bytes)",
                id, width, height, data_size);
    } else {
        /* VRAM full - keep in RAM for later promotion */
        S_WARN("VRAM full, texture {0} stored in RAM", id);
        existing->texture_ram = (uint8_t*)memalign(32, data_size);
        if(existing->texture_ram) {
            memcpy(existing->texture_ram, data, data_size);
        }
        existing->in_vram = false;
    }
#else
    /* Non-DC: just store in RAM */
    existing->texture_ram = (uint8_t*)malloc(data_size);
    if(existing->texture_ram) {
        memcpy(existing->texture_ram, data, data_size);
    }
#endif

    return id;
}

PVRTextureObject* PVRTextureManager::bind_texture(int id) {
    if(id == currently_bound_texture_) {
        auto tex = find_texture(id);
        if(tex) {
            textures_this_frame_.insert(tex.get());
            return tex.get();
        }
    }

    auto tex = find_texture(id);
    if(!tex) return nullptr;

    textures_this_frame_.insert(tex.get());
    currently_bound_texture_ = id;

    return tex.get();
}

void PVRTextureManager::release_texture(int id) {
    for(auto it = textures_.begin(); it != textures_.end(); ++it) {
        if((*it) && (*it)->id == id) {
#ifdef __DREAMCAST__
            if((*it)->texture_vram) {
                pvr_mem_free((pvr_ptr_t)(*it)->texture_vram);
            }
#endif
            if((*it)->texture_ram) {
                free((*it)->texture_ram);
            }

            /* Remove from priority list */
            texture_priority_.remove((*it).get());
            textures_this_frame_.erase((*it).get());

            textures_.erase(it);
            break;
        }
    }

    if(currently_bound_texture_ == id) {
        currently_bound_texture_ = 0;
    }
}

bool PVRTextureManager::evict_texture(PVRTextureObject* obj) {
#ifdef __DREAMCAST__
    if(!obj->in_vram || !obj->texture_vram) return false;

    /* Copy VRAM data back to RAM */
    if(!obj->texture_ram) {
        obj->texture_ram = (uint8_t*)memalign(32, obj->data_size);
        if(!obj->texture_ram) {
            S_ERROR("Failed to allocate RAM for texture eviction");
            return false;
        }
    }

    /* Copy from VRAM to RAM - PVR VRAM is memory-mapped */
    memcpy(obj->texture_ram, obj->texture_vram, obj->data_size);

    pvr_mem_free((pvr_ptr_t)obj->texture_vram);
    obj->texture_vram = nullptr;
    obj->in_vram = false;

    S_DEBUG("Evicted texture {0} from VRAM", obj->id);
    return true;
#else
    _S_UNUSED(obj);
    return false;
#endif
}

bool PVRTextureManager::promote_texture(PVRTextureObject* obj) {
#ifdef __DREAMCAST__
    if(obj->in_vram) return true;
    if(!obj->texture_ram) return false;

    pvr_ptr_t vram = pvr_mem_malloc(obj->data_size);
    if(!vram) return false;

    pvr_txr_load(obj->texture_ram, vram, obj->data_size);
    obj->texture_vram = vram;
    obj->in_vram = true;

    /* Free the RAM copy */
    free(obj->texture_ram);
    obj->texture_ram = nullptr;

    S_DEBUG("Promoted texture {0} to VRAM", obj->id);
    return true;
#else
    _S_UNUSED(obj);
    return false;
#endif
}

void PVRTextureManager::update_priorities() {
    /* Move textures used this frame to front of priority list */
    for(auto* tex : textures_this_frame_) {
        texture_priority_.remove(tex);
        texture_priority_.push_front(tex);
    }

#ifdef __DREAMCAST__
    /* Try to promote any textures used this frame that aren't in VRAM */
    for(auto* tex : textures_this_frame_) {
        if(!tex->in_vram && tex->texture_ram) {
            if(!promote_texture(tex)) {
                /* VRAM full - try evicting the least recently used texture */
                bool evicted = false;
                for(auto it = texture_priority_.rbegin(); it != texture_priority_.rend(); ++it) {
                    PVRTextureObject* candidate = *it;
                    /* Don't evict textures used this frame */
                    if(textures_this_frame_.count(candidate)) continue;
                    if(!candidate->in_vram) continue;

                    if(evict_texture(candidate)) {
                        evicted = true;
                        /* Try promoting again */
                        if(promote_texture(tex)) {
                            break;
                        }
                    }
                }
                if(!evicted) {
                    S_WARN("Could not promote texture {0} - VRAM full", tex->id);
                }
            }
        }
    }
#endif

    textures_this_frame_.clear();
    currently_bound_texture_ = 0;
}

} // namespace smlt