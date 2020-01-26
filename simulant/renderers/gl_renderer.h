#pragma once

#include <unordered_map>

#include "../types.h"
#include "../texture.h"
#include "../threads/mutex.h"

namespace smlt {

/*
 * Shared functionality between the GL1.x and GL2.x renderers
 * I hate mixin style classes (implementation inheritance) but as
 * we need to implement methods from the Renderer base class this
 * is the most straightforward way to share the code.
*/

class GLRenderer {
protected:
    GLRenderer(Window* window):
        win_(window) {}

    void on_texture_register(TextureID tex_id, TexturePtr texture);
    void on_texture_unregister(TextureID tex_id, Texture* texture);
    void on_texture_prepare(TexturePtr texture);

    uint32_t convert_texture_format(TextureFormat format);
    uint32_t convert_texel_type(TextureTexelType type);

    thread::Mutex texture_object_mutex_;
    std::unordered_map<TextureID, uint32_t> texture_objects_;

private:
    // Not called window_ to avoid name clashes in subclasses
    Window* win_;
};

}
