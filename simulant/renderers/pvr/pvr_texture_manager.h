#pragma once

#include "../../texture.h"
#include "../../utils/limited_vector.h"
#include "pvr_api.h"
#include <cstdint>
#include <list>
#include <vector>

namespace smlt {

class PVRRenderer;

struct PVRMipmap {
    uint32_t offset = 0;
    int w = 0;
    int h = 0;
};

typedef LimitedVector<PVRMipmap, 10> PVRMipmapVector;

struct PVRTextureObject {
    int id = 0;
    int format = -1;                 // e.g. GU_PSM_5650
    uint8_t* texture_ram = nullptr;  // If not-null, this texture is in ram
    uint8_t* texture_vram = nullptr; // If not-null, this texture is in vram    
    int width = 0;
    int height = 0;
    std::size_t data_size = 0;

    uint8_t* palette = nullptr;
    std::size_t palette_size = 0;
    PVRPaletteFormat palette_format = PVR_PALETTE_FORMAT_NONE;
    bool can_fit_in_vram = false;

    TextureFilter filter = TEXTURE_FILTER_POINT;
    PVRMipmapVector mipmaps;

    bool is_swizzled = false;
};

class PVRTextureManager {
public:
    PVRTextureManager(PVRRenderer* renderer);
    ~PVRTextureManager();

    int upload_texture(int id, PVRTexFormat format, int width, int height,
                       std::size_t data_size, const uint8_t* data,
                       const uint8_t* palette, std::size_t palette_size,
                       PVRPaletteFormat palette_format, bool do_mipmaps);

    void bind_texture(int id);
    void release_texture(int id);

    void update_priorities();

    std::shared_ptr<PVRTextureObject> find_texture(int texture);

private:
    friend void update_data_pointer(void* src, void* dst, void* data);

    PVRRenderer* renderer_ = nullptr;
    bool evict_texture(PVRTextureObject* obj);
    bool promote_texture(PVRTextureObject* texture);

    std::vector<std::shared_ptr<PVRTextureObject>> textures_;

    /* As we bind textures, we move them to the front of the priority
     * queue, and add them to the textures_this_frame set. Each frame
     * we go through the textures_this_frame_, work out how much space
     * we need to promote them all, and then reverse through the priority
     * list freeing textures until we we have enough space */
    std::list<PVRTextureObject*> texture_priority_;
    std::unordered_set<PVRTextureObject*> textures_this_frame_;

    int currently_bound_texture_ = 0;
};

} // namespace smlt
