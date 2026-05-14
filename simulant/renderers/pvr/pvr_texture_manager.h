#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <list>
#include <unordered_set>
#include <memory>

#include "../../assets/texture_flags.h"

#ifdef __DREAMCAST__
#include <dc/pvr.h>
#endif

namespace smlt {

class PVRRenderer;

struct PVRTextureObject {
    int id = 0;
    int format = -1;

    /* Pointer into PVR VRAM (allocated via pvr_mem_malloc) */
    void* texture_vram = nullptr;

    /* CPU-side copy for eviction/re-upload */
    uint8_t* texture_ram = nullptr;

    int width = 0;
    int height = 0;
    std::size_t data_size = 0;

    bool is_compressed = false;
    bool is_twiddled = false;
    bool has_mipmaps = false;

    TextureFilter filter = TEXTURE_FILTER_POINT;

    bool in_vram = false;
};

class PVRTextureManager {
public:
    PVRTextureManager(PVRRenderer* renderer);
    ~PVRTextureManager();

    /**
     * Upload texture data. If the texture already exists (by id), it is updated.
     * Returns the id.
     */
    int upload_texture(int id, int format, int width, int height,
                       std::size_t data_size, const uint8_t* data,
                       bool is_compressed, bool is_twiddled, bool has_mipmaps,
                       TextureFilter filter);

    /**
     * Returns the PVR VRAM pointer for the given texture, or nullptr.
     * Also sets up the polygon context texture fields.
     */
    PVRTextureObject* bind_texture(int id);

    /**
     * Release a texture, freeing VRAM and RAM.
     */
    void release_texture(int id);

    /**
     * Called once per frame before rendering. Promotes hot textures
     * to VRAM and evicts cold ones.
     */
    void update_priorities();

    std::shared_ptr<PVRTextureObject> find_texture(int id);

private:
    PVRRenderer* renderer_ = nullptr;

    bool evict_texture(PVRTextureObject* obj);
    bool promote_texture(PVRTextureObject* obj);

    std::vector<std::shared_ptr<PVRTextureObject>> textures_;
    std::list<PVRTextureObject*> texture_priority_;
    std::unordered_set<PVRTextureObject*> textures_this_frame_;

    int currently_bound_texture_ = 0;
};

} // namespace smlt