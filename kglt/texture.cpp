#include <cassert>

#include "texture.h"

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

void Texture::upload(bool free_after, bool generate_mipmaps, bool repeat) {
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

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(generate_mipmaps) {
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }

    glTexImage2D(
        GL_TEXTURE_2D,
        0, (bpp_ == 32)? GL_RGBA: GL_RGB,
        width_, height_, 0,
        (bpp_ == 32) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, &data_[0]
    );

    assert(glGetError() == GL_NO_ERROR);

    if(free_after) {
        free();
    }
}

void Texture::free() {
    data_.clear();
}

}
