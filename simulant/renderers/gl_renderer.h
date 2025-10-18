#pragma once

#include <unordered_map>

#include "renderer.h"
#include "../types.h"
#include "../texture.h"
#include "../threads/mutex.h"

namespace smlt {

constexpr const char* const LIGHT_POSITION_PROPERTY = "s_light_position";
constexpr const char* const LIGHT_COLOR_PROPERTY = "s_light_color";
constexpr const char* const LIGHT_INTENSITY_PROPERTY = "s_light_intensity";
constexpr const char* const LIGHT_RANGE_PROPERTY = "s_light_range";
constexpr const char* const LIGHT_COUNT_PROPERTY = "s_light_count";
constexpr const char* const VIEW_MATRIX_PROPERTY = "s_view";
constexpr const char* const MODELVIEW_PROJECTION_MATRIX_PROPERTY = "s_modelview_projection";
constexpr const char* const PROJECTION_MATRIX_PROPERTY = "s_projection";
constexpr const char* const MODELVIEW_MATRIX_PROPERTY = "s_modelview";
constexpr const char* const INVERSE_TRANSPOSE_MODELVIEW_MATRIX_PROPERTY = "s_inverse_transpose_modelview";

#ifdef __DREAMCAST__
// The Dreamcast only supports 2 multitexture units
#define _S_GL_MAX_TEXTURE_UNITS 1
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

class GLRenderer : public Renderer {
protected:
    GLRenderer(Window* window);

    void on_texture_register(AssetID tex_id, Texture *texture) override;
    void on_texture_unregister(AssetID tex_id, Texture* texture) override;
    void on_texture_prepare(Texture* texture) override;
    bool texture_format_is_native(TextureFormat fmt) override;

    uint32_t convert_format(TextureFormat format);
    uint32_t convert_type(TextureFormat format);

    thread::Mutex texture_object_mutex_;
    std::unordered_map<AssetID, uint32_t> texture_objects_;

    bool is_es() const {
        return is_es_;
    }

    void init_context() override;

    void apply_viewport(const RenderTarget& target,
                        const Viewport& viewport) override;
    void clear(const RenderTarget& target, const Color& colour,
               uint32_t clear_flags) override;

    void do_swap_buffers() override;

private:
    bool is_es_ = false;

};

}
