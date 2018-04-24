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

void Texture::set_format(TextureFormat format, TextureTexelType texel_type) {
    if(format == format_) {
        return;
    }

    format_ = format;

    // Default the texel type to what is probably required
    texel_type_ = (
        (texel_type == TEXTURE_TEXEL_TYPE_UNSPECIFIED) ?
        texel_type_from_texture_format(format) : texel_type
    );

    data_.resize(width_ * height_ * bytes_per_pixel());
    data_.shrink_to_fit();

    data_dirty_ = true;
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

static void explode_r8(uint8_t* source, const SourceChannelSet& channels, float& r, float& g, float& b, float& a) {
    float sr = float(*source) / 255.0f;

    auto calculate_component = [&channels](uint8_t i, float sr, float sg, float sb, float sa) -> float {
        switch(channels[i]) {
            case SOURCE_CHANNEL_ZERO: return 0.0f;
            case SOURCE_CHANNEL_ONE: return 1.0f;
            case SOURCE_CHANNEL_RED: return sr;
            case SOURCE_CHANNEL_GREEN: return sg;
            case SOURCE_CHANNEL_BLUE: return sb;
            case SOURCE_CHANNEL_ALPHA: return sa;
            default:
                return 0.0f;
        }
    };

    r = calculate_component(0, sr, 0, 0, 0);
    g = calculate_component(1, sr, 0, 0, 0);
    b = calculate_component(2, sr, 0, 0, 0);
    a = calculate_component(3, sr, 0, 0, 0);
}

static void compress_rgba4444(uint8_t* dest, float r, float g, float b, float a) {
    uint16_t* out = (uint16_t*) dest;

    uint8_t rr = (uint8_t) float(15) * r;
    uint8_t rg = (uint8_t) float(15) * g;
    uint8_t rb = (uint8_t) float(15) * b;
    uint8_t ra = (uint8_t) float(15) * a;

    *out = (rr << 12) | (rg << 8) | (rb << 4) | ra;
}

static void compress_rgba8888(uint8_t* dest, float r, float g, float b, float a) {
    uint32_t* out = (uint32_t*) dest;

    uint8_t rr = (uint8_t) float(255) * r;
    uint8_t rg = (uint8_t) float(255) * g;
    uint8_t rb = (uint8_t) float(255) * b;
    uint8_t ra = (uint8_t) float(255) * a;

    *out = (rr << 24) | (rg << 16) | (rb << 8) | ra;
}

typedef void (*ExplodeFunc)(uint8_t*, const SourceChannelSet&, float&, float&, float&, float&);
typedef void (*CompressFunc)(uint8_t*, float, float, float, float);

static const std::unordered_map<TextureFormat, ExplodeFunc> EXPLODERS = {
    {TEXTURE_FORMAT_R8, explode_r8}
};

static const std::unordered_map<TextureFormat, CompressFunc> COMPRESSORS = {
    {TEXTURE_FORMAT_RGBA4444, compress_rgba4444},
    {TEXTURE_FORMAT_RGBA8888, compress_rgba8888}
};

void Texture::convert(TextureFormat new_format, const SourceChannelSet &channels) {
    if(data_.empty()) {
        throw std::logic_error("Tried to convert a texture with no data");
    }

    auto original_format = format_;
    auto original_data = data_;

    set_format(new_format);

    if(original_format == TEXTURE_FORMAT_R8 && new_format == TEXTURE_FORMAT_RGBA4444) {
        auto source_stride = texture_format_stride(original_format);
        auto dest_stride = texture_format_stride(new_format);

        uint8_t* source_ptr = &original_data[0];
        uint8_t* dest_ptr = &data_[0];

        auto explode = EXPLODERS.at(original_format);
        auto compress = COMPRESSORS.at(new_format);

        for(auto i = 0u; i < original_data.size(); i += source_stride, source_ptr += source_stride, dest_ptr += dest_stride) {
            float r, g, b, a;

            explode(source_ptr, channels, r, g, b, a);
            compress(dest_ptr, r, g, b, a);
        }
    } else {
        throw std::logic_error("Unsupported texture conversion");
    }
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
    case TEXTURE_FORMAT_R8:
    case TEXTURE_FORMAT_RGB888:
    case TEXTURE_FORMAT_RGBA8888:
    case TEXTURE_FORMAT_RGBA4444:
    case TEXTURE_FORMAT_RGBA5551:
        return false;
    default:
        return true;
    }
}

std::size_t Texture::bits_per_pixel() const {
    switch(texel_type_) {
    case TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_4_4_4_4:
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
    case TEXTURE_FORMAT_R8:
        return 1;
    case TEXTURE_FORMAT_RGB888:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ_TWID:
    case TEXTURE_FORMAT_RGB_S3TC_DXT1_EXT:
        return 3;
    case TEXTURE_FORMAT_RGBA8888:
    case TEXTURE_FORMAT_RGBA4444:
    case TEXTURE_FORMAT_RGBA5551:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ_TWID:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ:
    case TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ_TWID:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT1_EXT:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT3_EXT:
    case TEXTURE_FORMAT_RGBA_S3TC_DXT5_EXT:
        return 4;
    default:        
        assert(0 && "Not implemented");
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

uint8_t texture_format_stride(TextureFormat format) {
    switch(format) {
    case TEXTURE_FORMAT_R8: return 1;
    case TEXTURE_FORMAT_RGBA4444:
    case TEXTURE_FORMAT_RGBA5551: return 2;
    case TEXTURE_FORMAT_RGB888: return 3;
    case TEXTURE_FORMAT_RGBA8888: return 4;
    default:
        assert(0 && "Not implemented");
        return 0;
    }
}

TextureTexelType texel_type_from_texture_format(TextureFormat format) {
    /* Given a texture format definition, return the texel type for it */

    switch(format) {
    case TEXTURE_FORMAT_R8:
    case TEXTURE_FORMAT_RGB888:
    case TEXTURE_FORMAT_RGBA8888:
        return TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE;
    case TEXTURE_FORMAT_RGBA4444:
        return TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_4_4_4_4;
    case TEXTURE_FORMAT_RGBA5551:
        return TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_5_5_1;
    default:
        assert(0 && "Not implemented");
        return TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE;
    }
}

}
