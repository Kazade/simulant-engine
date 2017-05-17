//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cassert>
#include <stdexcept>

#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "deps/kazlog/kazlog.h"
#include "deps/SOIL/SOIL.h"

#include "window_base.h"
#include "texture.h"
#include "resource_manager.h"

#ifdef _arch_dreamcast
#include <GL/gl.h>
#else

#ifdef SIMULANT_GL_VERSION_2X
#include "./renderers/gl2x/glad/glad/glad.h"
#else
#include "./renderers/gl1x/glad/glad/glad.h"
#endif

#endif //_arch_dreamcast

namespace smlt {

const std::string Texture::BuiltIns::CHECKERBOARD = "simulant/materials/textures/checkerboard.png";
const std::string Texture::BuiltIns::BUTTON = "simulant/materials/textures/button.png";

Texture::~Texture() {
    if(gl_tex_) {
        GLCheck(glDeleteTextures, 1, &gl_tex_);
    }
}

void Texture::set_bpp(uint32_t bits) {
    bpp_ = bits;
    resize(width_, height_);
}

void Texture::resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    data_.clear();
    data_.resize(width * height * (bpp_ / 8));
}

void Texture::sub_texture(TextureID src, uint16_t offset_x, uint16_t offset_y) {
    auto source_ptr = resource_manager().texture(src); //Lock

    //Bad things...
    Texture& source = *source_ptr;

    if(offset_x + source.width() > width() ||
        offset_y + source.height() > height()) {
        throw std::logic_error("Out of bounds error while blitting texture");
    }

    if(bpp() != source.bpp()) {
        throw std::logic_error("Tried to blit texture of a different colour depth");
    }

    for(uint16_t j = 0; j < source.height(); ++j) {
        for(uint16_t i = 0; i < source.width(); ++i) {
            uint16_t idx = ((width() * (offset_y + j)) + (offset_x + i)) * (bpp() / 8);
            uint16_t source_idx = ((source.width() * j) + i) * (bpp() / 8);

            data()[idx] = source.data()[source_idx];
            data()[idx+1] = source.data()[source_idx+1];
            data()[idx+2] = source.data()[source_idx+2];
            if(bpp() == 32) {
                data()[idx+3] = source.data()[source_idx+3];
            }
        }
    }

    //FIXME: SHould use glTexSubImage
    upload();
}

void Texture::__do_upload(MipmapGenerate mipmap, TextureWrap wrap, TextureFilter filter, bool free_after) {
    if(!gl_tex()) {
        GLCheck(glGenTextures, 1, &gl_tex_);
    }

    GLCheck(glBindTexture, GL_TEXTURE_2D, gl_tex_);
    GLCheck(glPixelStorei, GL_PACK_ALIGNMENT,1);
    GLCheck(glPixelStorei, GL_UNPACK_ALIGNMENT,1);

    // FIXME: This is awful, we should expose this via an API
    GLenum internalFormat, format;
    switch(bpp_) {
        case 32: {
            internalFormat = GL_RGBA;
            format = GL_RGBA;
        } break;
        case 24: {
            internalFormat = GL_RGB;
            format = GL_RGB;
        } break;
        case 8: {
            internalFormat = GL_RED;
            format = GL_RED;
        } break;
    default:
        assert(0 && "Not implemented");
    }

    GLCheck(glTexImage2D,
        GL_TEXTURE_2D,
        0, internalFormat,
        width_, height_, 0,
        format,
        GL_UNSIGNED_BYTE, &data_[0]
    );
    if(mipmap == MIPMAP_GENERATE_COMPLETE) {
#ifdef SIMULANT_GL_VERSION_1X
        // FIXME: OpenGL >= 1.4 - may need to look for GL_SGIS_generate_mipmap extension
        //GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
#else
        GLCheck(glGenerateMipmap, GL_TEXTURE_2D);
#endif
    }

    switch(wrap) {
        case TEXTURE_WRAP_REPEAT: {
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        } break;
        case TEXTURE_WRAP_CLAMP_TO_EDGE: {
#ifdef SIMULANT_GL_VERSION_2X
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#else
            //FIXME: check for extension and use GL_CLAMP_TO_EDGE
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
#endif
        } break;
    default:
        assert(0 && "Not Implemented");
    }

    if(filter == TEXTURE_FILTER_NEAREST) {
        if(mipmap == MIPMAP_GENERATE_COMPLETE) {
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        } else {
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    } else {
        if(mipmap == MIPMAP_GENERATE_COMPLETE) {
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        } else {
            GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    if(free_after) {
        free();
    }
}

void Texture::upload(MipmapGenerate mipmap, TextureWrap wrap, TextureFilter filter, bool free_after) {
    if(GLThreadCheck::is_current()) {
        __do_upload(mipmap, wrap, filter, free_after);
    } else {

        //FIXME: This might get hairy if more than one thread is messing with the texture
        //as we do an unlocked access here (which is fine when it's only this thread and the
        //main thread, but if there's another one then, that could be bad news)
        resource_manager().window->idle->add_once([=] {
            this->__do_upload(mipmap, wrap, filter, free_after);
        });

        //Wait for the main thread to process the upload
        resource_manager().window->idle->wait();
    }
}

void Texture::flip_vertically() {
    /**
     *  Flips the texture data vertically
     */

    for(uint32_t j = 0; j * 2 < (uint32_t) height(); ++j)
    {
        int index1 = j * width() * channels();
        int index2 = (height() - 1 - j) * width() * channels();
        for(uint32_t i = width() * channels(); i > 0; --i )
        {
            uint8_t temp = data_[index1];
            data_[index1] = data_[index2];
            data_[index2] = temp;
            ++index1;
            ++index2;
        }
    }
}

void Texture::free() {
    data_.clear();
}

void Texture::save_to_file(const unicode& filename) {
    SOIL_save_image(filename.encode().c_str(), SOIL_SAVE_TYPE_TGA, width(), height(), bpp() / 8, &data_[0]);
}

}
