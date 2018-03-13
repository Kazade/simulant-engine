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
#include "utils/memory.h"

#include "deps/kazlog/kazlog.h"
#include "deps/SOIL/SOIL.h"

#include "window.h"
#include "texture.h"
#include "resource_manager.h"
#include "renderers/renderer.h"

namespace smlt {

const std::string Texture::BuiltIns::CHECKERBOARD = "simulant/textures/checkerboard.png";
const std::string Texture::BuiltIns::BUTTON = "simulant/textures/button.png";


Texture::Texture(TextureID id, ResourceManager *resource_manager):
    Resource(resource_manager),
    generic::Identifiable<TextureID>(id),
    width_(0),
    height_(0) {

    renderer_ = resource_manager->window->renderer;
}

void Texture::set_texel_type(TextureTexelType type) {
    if(texel_type_ == type) {
        return;
    }

    texel_type_ = type;
    data_.resize(width_ * height_ * bytes_per_pixel());
    data_.shrink_to_fit();

    data_dirty_ = true;
}

void Texture::set_format(TextureFormat format) {
    format_ = format;
}

void Texture::resize(uint32_t width, uint32_t height, uint32_t data_size) {
    if(width_ == width && height_ == height && data_.size() == data_size) {
        return;
    }

    width_ = width;
    height_ = height;
    data_.resize(data_size);
    data_.shrink_to_fit();
    data_dirty_ = true;
}

void Texture::resize(uint32_t width, uint32_t height) {
    if(width_ == width && height_ == height) {
        return;
    }

    if(is_compressed()) {
        throw std::logic_error("Unable to resize a compressed texture with width and height");
    }

    width_ = width;
    height_ = height;

    data_.resize(width * height * bytes_per_pixel());
    data_.shrink_to_fit();

    data_dirty_ = true;
}

void Texture::flip_vertically() {
    /**
     *  Flips the texture data vertically
     */
    auto w = (uint32_t) width();
    auto h = (uint32_t) height();
    auto c = (uint32_t) channels();

    auto row_size = w * c;

    std::vector<uint8_t> new_data(data_.size());

    auto* in = &data_[0];
    auto* out = &new_data[row_size * (h - 1)];

    for(uint32_t j = 0; j < h; ++j) {
        std::memcpy(out, in, row_size);
        in += row_size;
        out -= row_size;
    }

    data_ = std::move(new_data);
}

void Texture::free() {
    data_.clear();
    data_.shrink_to_fit();
}

bool Texture::is_compressed() const {
    switch(format_) {
    case TEXTURE_FORMAT_RGB:
    case TEXTURE_FORMAT_RGBA:
        return false;
    default:
        return true;
    }
}

std::size_t Texture::bits_per_pixel() const {
    switch(texel_type_) {
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_4_4_4_4:
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_6_5:
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_5_5_1:
        // Packed into a short
        return sizeof(unsigned short) * 8;
    default:
        // byte per channel
        return sizeof(unsigned char) * channels() * 8;
    }
}

uint8_t Texture::channels() const {
    switch(format_) {
    case TEXTURE_FORMAT_RGB:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ_TWID:
    case TEXTURE_FORMAT_RGB_S3TC_DXT1_EXT:
        return 3;
    case TEXTURE_FORMAT_RGBA:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ_TWID:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ_TWID:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT1_EXT:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT3_EXT:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT5_EXT:
        return 4;
    default:
        // Shouldn't happen, but will be more obviously debuggable if it does
        return 0;
    }
}

const Texture::Data &Texture::data() const {
    return data_;
}

void Texture::save_to_file(const unicode& filename) {
    SOIL_save_image(filename.encode().c_str(), SOIL_SAVE_TYPE_TGA, width(), height(), bytes_per_pixel(), &data_[0]);
}

void Texture::set_texture_filter(TextureFilter filter) {
    if(filter != filter_) {
        filter_ = filter;
        params_dirty_ = true;
    }
}

void Texture::set_free_data_mode(TextureFreeData mode) {
    free_data_mode_ = mode;
}

void Texture::set_texture_wrap(TextureWrap wrap_u, TextureWrap wrap_v, TextureWrap wrap_w) {
    set_texture_wrap_u(wrap_u);
    set_texture_wrap_v(wrap_v);
    set_texture_wrap_w(wrap_w);
}

void Texture::set_texture_wrap_u(TextureWrap wrap_u) {
    if(wrap_u != wrap_u_) {
        wrap_u_ = wrap_u;
        params_dirty_ = true;
    }
}

void Texture::set_texture_wrap_v(TextureWrap wrap_v) {
    if(wrap_v != wrap_v_) {
        wrap_v_ = wrap_v;
        params_dirty_ = true;
    }
}

void Texture::set_texture_wrap_w(TextureWrap wrap_w) {
    if(wrap_w != wrap_w_) {
        wrap_w_ = wrap_w;
        params_dirty_ = true;
    }
}

bool Texture::init() {
    // Tell the renderer about the texture
    renderer_->register_texture(id(), shared_from_this());
    return true;
}

void Texture::cleanup() {
    // Tell the renderer to forget the texture
    renderer_->unregister_texture(id());
}

void Texture::update(float dt) {
    // Must only be called on the main thread as the renderer may do GL things
    assert(GLThreadCheck::is_current());

    // Should have been registered by now
    assert(renderer_->is_texture_registered(id()));

    // Make sure that the renderer is aware of any changes
    renderer_->prepare_texture(id());
}

TextureLock::TextureLock(Texture *tex, bool wait):
    tex_(tex) {

    if(wait) {
        tex_->mutex_.lock();
    } else {
        if(!tex_->mutex_.try_lock()) {

            // If we can't lock, wipe the tex lock (false-y)
            tex_ = nullptr;
            throw NoTextureLockError("Unable to lock the texture");
        }
    }
}

TextureLock::~TextureLock() {
    if(tex_) {
        tex_->mutex_.unlock();
    }
}

}
