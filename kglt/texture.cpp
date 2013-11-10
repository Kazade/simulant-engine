#include "utils/glcompat.h"

#include <cassert>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "kazbase/logging.h"

#include "window_base.h"
#include "texture.h"
#include "scene.h"

namespace kglt {

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
    Texture& source = *source_ptr.__object;

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

void Texture::__do_upload(bool free_after, bool generate_mipmaps, bool repeat, bool linear) {
    check_and_log_error(__FILE__, __LINE__);

    if(!gl_tex()) {
        GLCheck(glGenTextures, 1, &gl_tex_);
    }

    check_and_log_error(__FILE__, __LINE__);

    GLCheck(glBindTexture, GL_TEXTURE_2D, gl_tex_);
    check_and_log_error(__FILE__, __LINE__);

    GLCheck(glTexImage2D, 
        GL_TEXTURE_2D,
        0, (bpp_ == 32)? GL_RGBA: GL_RGB,
        width_, height_, 0,
        (bpp_ == 32) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, &data_[0]
    );
    check_and_log_error(__FILE__, __LINE__);

    if(generate_mipmaps) {
        GLCheck(glGenerateMipmap, GL_TEXTURE_2D);
    }
    check_and_log_error(__FILE__, __LINE__);

    if(repeat) {
        GLCheck(glTexParameterf, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        GLCheck(glTexParameterf, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        GLCheck(glTexParameterf, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        GLCheck(glTexParameterf, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    check_and_log_error(__FILE__, __LINE__);

    if(!linear) {
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GLCheck(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    check_and_log_error(__FILE__, __LINE__);


    if(free_after) {
        free();
    }
}

void Texture::upload(bool free_after, bool generate_mipmaps, bool repeat, bool linear) {
    if(GLThreadCheck::is_current()) {
        __do_upload(free_after, generate_mipmaps, repeat, linear);
    } else {

        //FIXME: This might get hairy if more than one thread is messing with the texture
        //as we do an unlocked access here (which is fine when it's only this thread and the
        //main thread, but if there's another one then, that could be bad news)
        resource_manager().window().idle().add_once([=] {
            this->__do_upload(free_after, generate_mipmaps, repeat, linear);
        });

        //Wait for the main thread to process the upload
        resource_manager().window().idle().wait();
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
            uint8_t temp = data()[index1];
            data()[index1] = data()[index2];
            data()[index2] = temp;
            ++index1;
            ++index2;
        }
    }
}

void Texture::free() {
    data_.clear();
}

}
