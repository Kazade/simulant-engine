#include "gl_renderer.h"

#include "../window.h"
#include "../application.h"
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

    /* PSP doesn't include these constants, we don't use them but need them for
     * compilation */

#define GL_PALETTE4_RGB8_OES              0x8B90
#define GL_PALETTE4_RGBA8_OES             0x8B91
#define GL_PALETTE4_R5_G6_B5_OES          0x8B92
#define GL_PALETTE4_RGBA4_OES             0x8B93
#define GL_PALETTE4_RGB5_A1_OES           0x8B94
#define GL_PALETTE8_RGB8_OES              0x8B95
#define GL_PALETTE8_RGBA8_OES             0x8B96
#define GL_PALETTE8_R5_G6_B5_OES          0x8B97
#define GL_PALETTE8_RGBA4_OES             0x8B98
#define GL_PALETTE8_RGB5_A1_OES           0x8B99

#else
    #include "./glad/glad/glad.h"
#endif


namespace smlt {

void GLRenderer::on_texture_register(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);

    GLuint gl_tex;

    S_DEBUG("Registering texture...");

    cr_run_main([&gl_tex]() {
        GLCheck(glGenTextures, 1, &gl_tex);
    });

    S_DEBUG("Setting the GL texture ID");
    texture->_set_renderer_specific_id(gl_tex);
}

void GLRenderer::on_texture_unregister(TextureID tex_id, Texture* texture) {
    _S_UNUSED(tex_id);

    GLuint gl_tex = texture->_renderer_specific_id();

    cr_run_main([&gl_tex]() {
        GLCheck(glDeleteTextures, 1, &gl_tex);
    });

    texture->_set_renderer_specific_id(0);
}

uint32_t GLRenderer::convert_format(TextureFormat format) {
    switch(format) {
        case TEXTURE_FORMAT_R_1UB_8:
            return GL_RED;
        case TEXTURE_FORMAT_RGB_3UB_888:
        case TEXTURE_FORMAT_RGB_1US_565:
        case TEXTURE_FORMAT_RGB565_PALETTED4:
        case TEXTURE_FORMAT_RGB565_PALETTED8:
        case TEXTURE_FORMAT_RGB8_PALETTED4:
        case TEXTURE_FORMAT_RGB8_PALETTED8:
            return GL_RGB;
        case TEXTURE_FORMAT_ARGB_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_1555:
            return GL_BGRA;
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_RGBA_1US_5551:
        case TEXTURE_FORMAT_RGBA_4UB_8888:
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
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
    case TEXTURE_FORMAT_RGB565_PALETTED4:
    case TEXTURE_FORMAT_RGB565_PALETTED8:
    case TEXTURE_FORMAT_RGB8_PALETTED4:
    case TEXTURE_FORMAT_RGB8_PALETTED8:
    case TEXTURE_FORMAT_RGBA8_PALETTED4:
    case TEXTURE_FORMAT_RGBA8_PALETTED8:
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
        auto data = &texture->data()[0];

#if defined(__DREAMCAST__)
        bool hardware_palettes_supported = true;
#elif defined(__PSP__)
        /* FIXME: Does PSP GL support this? Unlikely... */
        bool hardware_palettes_supported = false;
#else
        bool hardware_palettes_supported = GLAD_GL_OES_compressed_paletted_texture;
#endif
        bool paletted = texture->is_paletted_format();
        std::vector<uint8_t> new_data;

        if(paletted) {
            /* Paletted textures need some additional work. If we can support them
             * in hardware, we do, otherwise the data gets copied to the paletted_data
             * and unpacked before upload */
            if(hardware_palettes_supported) {
                /* We can upload directly! Set the types appropriately!*/
                format = (f == TEXTURE_FORMAT_RGB565_PALETTED4) ? GL_PALETTE4_R5_G6_B5_OES :
                          (f == TEXTURE_FORMAT_RGB565_PALETTED8) ? GL_PALETTE8_R5_G6_B5_OES :
                          (f == TEXTURE_FORMAT_RGB8_PALETTED4) ? GL_PALETTE4_RGB8_OES :
                          (f == TEXTURE_FORMAT_RGB8_PALETTED8) ? GL_PALETTE8_RGB8_OES :
                          (f == TEXTURE_FORMAT_RGBA8_PALETTED4) ? GL_PALETTE4_RGBA8_OES :
                          GL_PALETTE4_RGBA8_OES;
                type = GL_UNSIGNED_BYTE;  /* Is this correct??? */
            } else {
                /* It's unpack time! */
                uint8_t* palette = texture->_stash_paletted_data();
                uint8_t* indexed = palette + texture->palette_size();

                auto half_byte = (f == TEXTURE_FORMAT_RGB565_PALETTED4 || f == TEXTURE_FORMAT_RGB8_PALETTED4 || f == TEXTURE_FORMAT_RGBA8_PALETTED4);
                auto texel_size = (f == TEXTURE_FORMAT_RGB565_PALETTED4 || f == TEXTURE_FORMAT_RGB565_PALETTED8) ? 2 :
                                  (f == TEXTURE_FORMAT_RGB8_PALETTED4 || f == TEXTURE_FORMAT_RGB8_PALETTED4) ? 3 : 4;

                for(int i = 0; i < (texture->width() * texture->height()) / 2; ++i) {
                    uint8_t current_byte = indexed[i];

                    if(half_byte) {
                        uint8_t index = (current_byte & 0xF0) >> 4;
                        for(int j = 0; j < texel_size; ++j) {
                            new_data.push_back(palette[(index * texel_size) + j]);
                        }

                        index = (current_byte & 0x0F) >> 0;

                        for(int j = 0; j < texel_size; ++j) {
                            new_data.push_back(palette[(index * texel_size) + j]);
                        }
                    } else {
                        for(int j = 0; j < texel_size; ++j) {
                            new_data.push_back(palette[(current_byte * texel_size) + j]);
                        }
                    }
                }

                /* We want to use the new data, not what was uploaded by the user. That
                 * data is now stashed in the palette */
                data = &new_data[0];
                format = internal_format = (texel_size == 4) ? GL_RGBA : GL_RGB;
                type = (texel_size == 2) ? GL_UNSIGNED_SHORT_5_6_5 : GL_UNSIGNED_BYTE;
            }
        }

        if(format > 0 && type > 0) {
            /* Paletted textures are uploaded using glCompressedTexImage2D if the
             * OES_compressed_paletted_texture extension is available */

            if(texture->is_compressed() || (hardware_palettes_supported && paletted)) {
                GLCheck(glCompressedTexImage2D,
                    GL_TEXTURE_2D,
                    0,
                    format,
                    texture->width(), texture->height(), 0,
                    texture->data_size(),
                    data
                );

                if(texture_format_contains_mipmaps(f)) {
                    texture->_set_has_mipmaps(true);
                }
            } else {
                GLCheck(glTexImage2D,
                    GL_TEXTURE_2D,
                    0, internal_format,
                    texture->width(), texture->height(), 0,
                    format,
                    type, data
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
                S_DEBUG("Mipmaps generated");
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
                if(texture->has_mipmaps()) {
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
                } else {
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                }
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

bool GLRenderer::texture_format_is_native(TextureFormat fmt) {
    /* We handle conversion of paletted textures automatically */
    if(fmt == TEXTURE_FORMAT_RGB565_PALETTED4 ||
        fmt == TEXTURE_FORMAT_RGB565_PALETTED8 ||
        fmt == TEXTURE_FORMAT_RGB8_PALETTED4 ||
        fmt == TEXTURE_FORMAT_RGB8_PALETTED8 ||
        fmt == TEXTURE_FORMAT_RGBA8_PALETTED4 ||
        fmt == TEXTURE_FORMAT_RGBA8_PALETTED8) {
        return true;
    }

    return Renderer::texture_format_is_native(fmt);
}

}
