#pragma once

#include "../../texture.h"
#include "../../utils/limited_vector.h"
#include <cstdint>
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

    /* When this texture is bound, we increase the priority. Before
     * each frame we decrease the priority by 1 until we hit min-priority.
     * If we run out of VRAM we find any resident textures with the lowest
     * priority and then move the largest ones out of vram. When a texture
     * is moved into vram its priority goes back to zero.
     */
    uint8_t priority = 255;

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

    PSPTextureObject* find_texture(int texture);

private:
    friend void update_data_pointer(void* src, void* dst, void* data);

    PSPRenderer* renderer_ = nullptr;
    void evict_texture(PSPTextureObject* obj);
    void promote_texture(PSPTextureObject* texture);    

    std::vector<PSPTextureObject> textures_;
    int lowest_texture_priority_ = 0;

    /* Returns true if there are enough textures in vram with a lower
     * priority to make room for this one */
    bool space_in_vram(PSPTextureObject* obj);

    int currently_bound_texture_ = 0;
};

} // namespace smlt
