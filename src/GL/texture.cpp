
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
    if(free_after) {
        free();
    }
}

void Texture::free() {
    data_.clear();
}

}
