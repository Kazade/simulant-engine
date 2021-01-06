#pragma once

#include <unordered_map>

#include "../types.h"
#include "../texture.h"
#include "../threads/mutex.h"

namespace smlt {

#ifdef __DREAMCAST__
// The Dreamcast only supports 2 multitexture units
#define _S_GL_MAX_TEXTURE_UNITS 2
#elif defined(__PSP__)
// PSPGL doesn't support multitexturing at all (!)
#define _S_GL_MAX_TEXTURE_UNITS 1
#else
#define _S_GL_MAX_TEXTURE_UNITS 8
#endif

#define _S_GL_SUPPORTS_MULTITEXTURE (_S_GL_MAX_TEXTURE_UNITS > 1)

#ifdef __PSP__
#define _S_GL_SUPPORTS_COLOR_MATERIAL 0
#else
#define _S_GL_SUPPORTS_COLOR_MATERIAL 1
#endif

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

    void on_texture_register(TextureID tex_id, Texture *texture);
    void on_texture_unregister(TextureID tex_id, Texture* texture);
    void on_texture_prepare(Texture* texture);

    uint32_t convert_texture_format(TextureFormat format);
    uint32_t convert_texel_type(TextureTexelType type);

    thread::Mutex texture_object_mutex_;
    std::unordered_map<TextureID, uint32_t> texture_objects_;

private:
    // Not called window_ to avoid name clashes in subclasses
    Window* win_;
};

}
