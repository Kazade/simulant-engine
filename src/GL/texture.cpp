
#include "texture.h"

namespace GL {

Texture::~Texture() {

}

void Texture::set_bpp(int bits) {
    bpp_ = bits;
    resize(width_, height_);
}

void Texture::resize(int width, int height) {
    width_ = width;
    height_ = height;

    data_.clear();
    data_.resize(width * height * (bpp_ / 8));
}

void Texture::upload(bool free_after) {
    if(!gl_tex()) {
        glGenTextures(1, &gl_tex_);
    }

    glBindTexture(GL_TEXTURE_2D, gl_tex_);
    glTexImage2D(
        GL_TEXTURE_2D,
        0, bpp_ / 8,
        width_, height_, 0,
        (bpp_ == 32) ? GL_RGBA : GL_RGB,
        GL_UNSIGNED_BYTE, &data_[0]
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if(free_after) {
        free();
    }
}

void Texture::free() {
    data_.clear();
}

}
