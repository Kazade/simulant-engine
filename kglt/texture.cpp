#include <cassert>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include "kazbase/logging/logging.h"

#include "glee/GLee.h"
#include "texture.h"
#include "scene.h"

namespace kglt {

Texture::~Texture() {
    if(gl_tex_) {
        glDeleteTextures(1, &gl_tex_);
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
    Texture& source = resource_manager().texture(src);

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

void Texture::upload(bool free_after, bool generate_mipmaps, bool repeat, bool linear) {
    assert(glGetError() == GL_NO_ERROR);

    if(!gl_tex()) {
        glGenTextures(1, &gl_tex_);
    }

    glBindTexture(GL_TEXTURE_2D, gl_tex_);

    if(repeat) {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    } else {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    if(!linear) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if(generate_mipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }

    assert(glGetError() == GL_NO_ERROR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0, (bpp_ == 32)? GL_RGBA: GL_RGB,
        width_, height_, 0,
        (bpp_ == 32) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, &data_[0]
    );

    int error = glGetError();
    if(error != GL_NO_ERROR) {
        throw std::runtime_error("OpenGL error: " + boost::lexical_cast<std::string>(error));
    }

    if(free_after) {
        free();
    }
}

void Texture::free() {
    data_.clear();
}

}
