#include "gl_renderer.h"

#include "../window.h"
#include "../utils/gl_error.h"
#include "../utils/gl_thread_check.h"


/* This file should only contain things shared between GL1 + GL2 so include
 * the gl1 headers here */
#ifdef __DREAMCAST__
    #include "../../../deps/libgl/include/GL/gl.h"
    #include "../../../deps/libgl/include/GL/glext.h"
    #include "../../../deps/libgl/include/GL/glkos.h"
#elif defined(__PSP__)
    #include <GL/gl.h>
#else
    #include "./glad/glad/glad.h"
#endif


namespace smlt {

void GLRenderer::on_texture_register(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);

    GLuint gl_tex;

    S_DEBUG("Registering texture...");

    if(cort::within_coroutine()) {
        /* If we're in a coroutine, we need to make sure
         * we run the GL function on the idle task manager
         * and then yield. FIXME: When/if coroutines
         * aren't implemented using threads we won't
         * need to do this */
        S_DEBUG("In a coroutine, sending glGenTextures to main thread");
        win_->idle->add_once([&gl_tex]() {
            GLCheck(glGenTextures, 1, &gl_tex);
        });
        cort::yield_coroutine();
    } else {
        S_DEBUG("Generating a texture with GL");
        GLCheck(glGenTextures, 1, &gl_tex);
    }

    S_DEBUG("Setting the GL texture ID");
    texture->_set_renderer_specific_id(gl_tex);
}

void GLRenderer::on_texture_unregister(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);

    GLuint gl_tex = texture->_renderer_specific_id();

    if(cort::within_coroutine()) {
        win_->idle->add_once([&gl_tex]() {
            GLCheck(glDeleteTextures, 1, &gl_tex);
        });
        cort::yield_coroutine();
    } else {
        GLCheck(glDeleteTextures, 1, &gl_tex);
    }

    texture->_set_renderer_specific_id(0);

}

uint32_t GLRenderer::convert_format(TextureFormat format) {
    switch(format) {
        case TEXTURE_FORMAT_R_1UB_8:
            return GL_RED;
        case TEXTURE_FORMAT_RGB_3UB_888:
        case TEXTURE_FORMAT_RGB_1US_565:
            return GL_RGB;
        case TEXTURE_FORMAT_ARGB_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_1555:
            return GL_BGRA;
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_RGBA_1US_5551:
        case TEXTURE_FORMAT_RGBA_4UB_8888:
            return GL_RGBA;
#ifdef __DREAMCAST__
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
            return GL_COMPRESSED_ARGB_1555_VQ_TWID_KOS;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
            return GL_COMPRESSED_ARGB_4444_VQ_TWID_KOS;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
            return GL_COMPRESSED_RGB_565_VQ_TWID_KOS;
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
            return GL_COMPRESSED_ARGB_1555_VQ_MIPMAP_TWID_KOS;
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
            return GL_COMPRESSED_ARGB_4444_VQ_MIPMAP_TWID_KOS;
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
            return GL_COMPRESSED_RGB_565_VQ_MIPMAP_TWID_KOS;
#endif
        default:
            assert(0 && "Not implemented");
            return GL_RGBA;
    }
}

uint32_t GLRenderer::convert_type(TextureFormat format) {
    switch(format) {
    case TEXTURE_FORMAT_R_1UB_8:
    case TEXTURE_FORMAT_RGB_3UB_888:
    case TEXTURE_FORMAT_RGBA_4UB_8888:
        return GL_UNSIGNED_BYTE;
    case TEXTURE_FORMAT_RGB_1US_565:
        return GL_UNSIGNED_SHORT_5_6_5;
    case TEXTURE_FORMAT_RGBA_1US_4444:
        return GL_UNSIGNED_SHORT_4_4_4_4;
    case TEXTURE_FORMAT_RGBA_1US_5551:
        return GL_UNSIGNED_SHORT_5_5_5_1;
    case TEXTURE_FORMAT_ARGB_1US_1555:
        return GL_UNSIGNED_SHORT_1_5_5_5_REV;
    case TEXTURE_FORMAT_ARGB_1US_4444:
        return GL_UNSIGNED_SHORT_4_4_4_4_REV;
#ifdef __DREAMCAST__
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        /* Not used for anything, but return something sensible */
        return GL_UNSIGNED_SHORT;
#endif
    default:
        assert(0 && "Not implemented");
        return 0;
    }
}

static constexpr GLenum texture_format_to_internal_format(TextureFormat format) {
    return (format == TEXTURE_FORMAT_R_1UB_8) ? GL_RED :
           (format == TEXTURE_FORMAT_RGB_3UB_888) ? GL_RGB :
            GL_RGBA;
}

void GLRenderer::on_texture_prepare(Texture *texture) {
    // Do nothing if everything is up to date
    if(!texture->_data_dirty() && !texture->_params_dirty()) {
        return;
    }

    GLint active;
    GLuint target = texture->_renderer_specific_id();
    GLCheck(glGetIntegerv, GL_TEXTURE_BINDING_2D, &active);
    GLCheck(glBindTexture, GL_TEXTURE_2D, target);

    /* Only upload data if it's enabled on the texture */
    if(texture->_data_dirty() && texture->auto_upload()) {
        // Upload

        auto f = texture->format();
        auto format = convert_format(f);
        auto internal_format = texture_format_to_internal_format(f);
        auto type = convert_type(f);

        if(format > 0 && type > 0) {
            if(texture->is_compressed()) {
                GLCheck(glCompressedTexImage2D,
                    GL_TEXTURE_2D,
                    0,
                    format,
                    texture->width(), texture->height(), 0,
                    texture->data_size(),
                    &texture->data()[0]
                );
            } else {
                GLCheck(glTexImage2D,
                    GL_TEXTURE_2D,
                    0, internal_format,
                    texture->width(), texture->height(), 0,
                    format,
                    type, &texture->data()[0]
                );

                if(texture_format_contains_mipmaps(f)) {
                    S_WARN(
                        "Upload of provided mipmap texture data is not"
                        "currently implemented."
                    );

                    /* FIXME: call glTexImage for each mipmap level
                     * in the data. and then call
                     * texture_->_set_has_mipmaps(true);
                     */
                }
            }
        } else {
            // If the format isn't supported, don't upload anything, but warn about it!
            S_WARN_ONCE("Tried to use unsupported texture format in the GL renderer");
        }

        /* Free the data if that's what is wanted */
        if(texture->free_data_mode() == TEXTURE_FREE_DATA_AFTER_UPLOAD) {
            texture->free();
        }

        /* Generate mipmaps if we don't have them already */
        if(texture->mipmap_generation() == MIPMAP_GENERATE_COMPLETE && !texture->has_mipmaps() && !texture->is_compressed()) {
#ifdef __DREAMCAST__
            if(texture->width() == texture->height()) {
#endif

#ifdef __PSP__
                S_INFO("Not generating mipmaps as PSP doesn't support glGenerateMipmap");
#else

                S_DEBUG("Generating mipmaps. W: {0}, H:{1}",
                    texture->width(), texture->height()
                );
                GLCheck(glGenerateMipmapEXT, GL_TEXTURE_2D);
                texture->_set_has_mipmaps(true);
#endif

#ifdef __DREAMCAST__
            } else {
                S_WARN("Not generating mipmaps as texture is non-square (PVR limitation)");
            }
#endif
        }

        texture->_set_data_clean();
    }

    if(texture->_params_dirty()) {
        auto filter_mode = texture->texture_filter();
        switch(filter_mode) {
            case TEXTURE_FILTER_TRILINEAR: {
                if(!texture->has_mipmaps()) {
                    // Same as bilinear
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                } else {
                    // Trilinear
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                }

            } break;
            case TEXTURE_FILTER_BILINEAR: {
                GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            } break;
            case TEXTURE_FILTER_POINT:
            default: {
                GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            }
        }

        auto convert_wrap_mode = [](TextureWrap wrap) -> GLenum {
            switch(wrap) {
                case TEXTURE_WRAP_CLAMP_TO_EDGE:
                    return GL_CLAMP_TO_EDGE;
                break;
                default:
                    // FIXME: Implement other modes
                    return GL_REPEAT;
            }
        };

        auto wrapu = convert_wrap_mode(texture->wrap_u());
        auto wrapv = convert_wrap_mode(texture->wrap_v());

        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapu);
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapv);

        texture->_set_params_clean();
    }

    if(active != (GLint) target) {
        GLCheck(glBindTexture, GL_TEXTURE_2D, active);
    }
}

}
