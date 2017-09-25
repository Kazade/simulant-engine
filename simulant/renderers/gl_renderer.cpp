#include "gl_renderer.h"

#include "../window.h"
#include "../utils/gl_error.h"
#include "../utils/gl_thread_check.h"

namespace smlt {

void GLRenderer::on_texture_register(TextureID tex_id, TexturePtr texture) {
    GLuint gl_tex;

    if(!GLThreadCheck::is_current()) {
        win_->idle->run_sync([&gl_tex]() {
            GLCheck(glGenTextures, 1, &gl_tex);
        });
    } else {
        GLCheck(glGenTextures, 1, &gl_tex);

    }

    std::lock_guard<std::mutex> lock(texture_object_mutex_);
    texture_objects_[tex_id] = gl_tex;
}

void GLRenderer::on_texture_unregister(TextureID tex_id) {
    GLuint gl_tex;
    {
        std::lock_guard<std::mutex> lock(texture_object_mutex_);
        gl_tex = texture_objects_.at(tex_id);
        texture_objects_.erase(tex_id);
    }

    if(!GLThreadCheck::is_current()) {
        win_->idle->run_sync([&gl_tex]() {
            GLCheck(glDeleteTextures, 1, &gl_tex);
        });
    } else {
        GLCheck(glDeleteTextures, 1, &gl_tex);
    }
}


GLenum GLRenderer::convert_texture_format(TextureFormat format) {
    switch(format) {
        case TEXTURE_FORMAT_RGB:
            return GL_RGB;
        case TEXTURE_FORMAT_RGBA:
            return GL_RGBA;
        default:
            return 0;
    }
}

GLenum GLRenderer::convert_texel_type(TextureTexelType type) {
    switch(type) {
    case TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE:
        return GL_UNSIGNED_BYTE;
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_4_4_4_4:
        return GL_UNSIGNED_SHORT_4_4_4_4;
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_5_5_1:
        return GL_UNSIGNED_SHORT_5_5_5_1;
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_6_5:
        return GL_UNSIGNED_SHORT_5_6_5;
    default:
        return 0;
    }
}

void GLRenderer::on_texture_prepare(TexturePtr texture) {
    auto lock = texture->try_lock();
    if(!lock) {
        // Don't update anything unless we can get a lock
        return;
    }

    // Do nothing if everything is up to date
    if(!texture->_data_dirty() && !texture->_params_dirty()) {
        return;
    }

    GLint active;
    GLCheck(glGetIntegerv, GL_TEXTURE_BINDING_2D, &active);

    GLuint target;

    {
        std::lock_guard<std::mutex> lock(texture_object_mutex_);
        target = texture_objects_.at(texture->id());
    }

    GLCheck(glBindTexture, GL_TEXTURE_2D, target);

    if(texture->_data_dirty()) {
        // Upload
        auto format = convert_texture_format(texture->format());
        auto type = convert_texel_type(texture->texel_type());

        if(format > 0 && type > 0) {
            if(texture->is_compressed()) {
                GLCheck(glCompressedTexImage2D,
                    GL_TEXTURE_2D,
                    0,
                    format,
                    texture->width(), texture->height(), 0,
                    texture->data().size(),
                    &texture->data()[0]
                );
            } else {
                GLCheck(glTexImage2D,
                    GL_TEXTURE_2D,
                    0, format,
                    texture->width(), texture->height(), 0,
                    format,
                    type, &texture->data()[0]
                );
            }

            if(texture->mipmap_generation() == MIPMAP_GENERATE_COMPLETE) {
#ifdef SIMULANT_GL_VERSION_2X
                GLCheck(glGenerateMipmap, GL_TEXTURE_2D);
                texture->_set_has_mipmaps(true);
#endif
            }

        } else {
            // If the format isn't supported, don't upload anything, but warn about it!
            L_WARN_ONCE("Tried to use unsupported texture format in the GL renderer");
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
