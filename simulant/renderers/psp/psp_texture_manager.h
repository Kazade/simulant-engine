#pragma once

#include "../../texture.h"
#include "../../utils/limited_vector.h"
#include <cstdint>
#include <list>
#include <vector>

namespace smlt {

class PSPRenderer;

struct PSPMipmap {
    uint32_t offset = 0;
    int w = 0;
    int h = 0;
};

typedef LimitedVector<PSPMipmap, 10> PSPMipmapVector;

struct PSPTextureObject {
    int id = 0;
    int format = -1;                 // e.g. GU_PSM_5650
    uint8_t* texture_ram = nullptr;  // If not-null, this texture is in ram
    uint8_t* texture_vram = nullptr; // If not-null, this texture is in vram    
    int width = 0;
    int height = 0;
    std::size_t data_size = 0;

    uint8_t* palette = nullptr;
    std::size_t palette_size = 0;
    int palette_format = 0;
    bool can_fit_in_vram = false;

    TextureFilter filter = TEXTURE_FILTER_POINT;
    PSPMipmapVector mipmaps;

    bool is_swizzled = false;
};

class PSPTextureManager {
public:
    PSPTextureManager(PSPRenderer* renderer);
    ~PSPTextureManager();

    int upload_texture(int id, int format, int width, int height,
                       std::size_t data_size, const uint8_t* data,
                       const uint8_t* palette, std::size_t palette_size,
                       int palette_format, bool do_mipmaps);

    void bind_texture(int id);
    void release_texture(int id);

    void update_priorities();

    std::shared_ptr<PSPTextureObject> find_texture(int texture);

private:
    friend void update_data_pointer(void* src, void* dst, void* data);

    PSPRenderer* renderer_ = nullptr;
    bool evict_texture(PSPTextureObject* obj);
    bool promote_texture(PSPTextureObject* texture);

    std::vector<std::shared_ptr<PSPTextureObject>> textures_;

    /* As we bind textures, we move them to the front of the priority
     * queue, and add them to the textures_this_frame set. Each frame
     * we go through the textures_this_frame_, work out how much space
     * we need to promote them all, and then reverse through the priority
     * list freeing textures until we we have enough space */
    std::list<PSPTextureObject*> texture_priority_;
    std::unordered_set<PSPTextureObject*> textures_this_frame_;

    int currently_bound_texture_ = 0;
};

} // namespace smlt
