//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include <cassert>
#include <cstring>
#include <stdexcept>

#include "utils/gl_thread_check.h"
#include "utils/gl_error.h"

#include "logging.h"
#include "application.h"
#include "window.h"
#include "texture.h"
#include "asset_manager.h"
#include "renderers/renderer.h"

namespace smlt {

const std::string Texture::BuiltIns::CHECKERBOARD = "textures/checkerboard.png";
const std::string Texture::BuiltIns::BUTTON = "textures/button.png";

const TextureChannelSet Texture::DEFAULT_SOURCE_CHANNELS = {{
    TEXTURE_CHANNEL_RED,
    TEXTURE_CHANNEL_GREEN,
    TEXTURE_CHANNEL_BLUE,
    TEXTURE_CHANNEL_ALPHA
}};

bool texture_format_contains_mipmaps(TextureFormat format) {
    switch(format) {
    /* FIXME: We don't currently extract mipmap data from these formats on
     * non-Dreamcast platforms. We should fix this! */
#ifdef __DREAMCAST__
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
            return true;
#endif
        default:
            return false;
    }
}

std::size_t texture_format_channels(TextureFormat format) {
    switch(format) {
    case TEXTURE_FORMAT_R_1UB_8:
        return 1;
    case TEXTURE_FORMAT_RGB_1US_565:
    case TEXTURE_FORMAT_RGB_1US_565_TWID:
    case TEXTURE_FORMAT_RGB_3UB_888:
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
        return 3;
    case TEXTURE_FORMAT_RGBA_4UB_8888:
    case TEXTURE_FORMAT_RGBA_1US_4444:
    case TEXTURE_FORMAT_ARGB_1US_4444:
    case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
    case TEXTURE_FORMAT_RGBA_1US_5551:
    case TEXTURE_FORMAT_ARGB_1US_1555:
    case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
        return 4;
    default:
        S_ERROR("Invalid TextureFormat!");
        return 0;
    }
}

Texture::Texture(AssetID id, AssetManager *asset_manager, uint16_t width, uint16_t height, TextureFormat format):
    Asset(asset_manager),
    generic::Identifiable<AssetID>(id),
    width_(width),
    height_(height) {

    S_DEBUG("Creating texture {0}x{1}", width, height);
    S_DEBUG("Setting format to: {0}", format);
    set_format(format);  /* This will allocate the data */

    /* We intentionally don't mark data dirty here. All that would happen is
     * we would upload a blank texture */

    renderer_ = get_app()->window->renderer;
}

Texture::~Texture() {
    if(paletted_data_) {
        delete [] paletted_data_;
        paletted_data_ = nullptr;
    }

    free();
}

TextureFormat Texture::format() const {
    return format_;
}

uint16_t Texture::width() const {
    return width_;
}

uint16_t Texture::height() const {
    return height_;
}

std::size_t Texture::required_data_size(TextureFormat fmt, uint16_t width, uint16_t height) {   
    auto calc_vq_mipmap_size = [](std::size_t s) -> std::size_t {
        std::size_t ret = 0;
        while(s != 1) {
            ret += (s / 2) * (s / 2);
            s /= 2;
        }

        ret += (1 * 1);
        return ret;
    };

    switch(fmt) {
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
            /* 2048 byte codebook, 8bpp per 2x2 */
            return 2048 + ((width / 2) * (height / 2));
        case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
        case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
            return 2048 + calc_vq_mipmap_size(width);
        case TEXTURE_FORMAT_RGB565_PALETTED4:
            return (2 * 16) + ((width * height) / 2);
        case TEXTURE_FORMAT_RGB565_PALETTED8:
            return (2 * 256) + ((width * height));
        case TEXTURE_FORMAT_RGB8_PALETTED4:
            return (3 * 16) + ((width * height) / 2);
        case TEXTURE_FORMAT_RGB8_PALETTED8:
            return (3 * 256) + ((width * height));
        case TEXTURE_FORMAT_RGBA8_PALETTED4:
            return (4 * 16) + ((width * height) / 2);
        case TEXTURE_FORMAT_RGBA8_PALETTED8:
            return (4 * 256) + ((width * height));
        default:
            break;
    }

    return texture_format_stride(fmt) * width * height;
}

void Texture::set_format(TextureFormat format) {
    if(data_ && format_ == format) {
        return;
    }

    format_ = format;

    auto byte_size = required_data_size(format, width_, height_);
    resize_data(byte_size);
}

bool Texture::is_paletted_format() const {
    switch(format_) {
    case TEXTURE_FORMAT_RGB565_PALETTED4:
    case TEXTURE_FORMAT_RGB565_PALETTED8:
    case TEXTURE_FORMAT_RGB8_PALETTED4:
    case TEXTURE_FORMAT_RGB8_PALETTED8:
    case TEXTURE_FORMAT_RGBA8_PALETTED4:
    case TEXTURE_FORMAT_RGBA8_PALETTED8:
        return true;
    default:
        return false;
    }
}

uint32_t Texture::palette_size() const {
    if(!is_paletted_format()) {
        return 0;
    }

    if(format_ == TEXTURE_FORMAT_RGB565_PALETTED4) {
        return 16 * 2;
    } else if(format_ == TEXTURE_FORMAT_RGB565_PALETTED8) {
        return 256 * 2;
    } else if(format_ == TEXTURE_FORMAT_RGB8_PALETTED4) {
        return 16 * 3;
    } else if(format_ == TEXTURE_FORMAT_RGB8_PALETTED8) {
        return 256 * 3;
    } else if(format_ == TEXTURE_FORMAT_RGBA8_PALETTED4) {
        return 16 * 4;
    } else if(format_ == TEXTURE_FORMAT_RGBA8_PALETTED8) {
        return 256 * 4;
    }

    assert(0 && "Unhandled paletted format");
    return 0;
}

smlt::optional<Pixel> Texture::pixel(std::size_t x, std::size_t y) {
    // FIXME: this won't account for aligned formats (where rows are aligned to
    // particular boundaries) - we need to add a row_stride() to texture */
    auto stride = texture_format_stride(format_);
    uint8_t* src = data_ + ((y * width() * stride) + (x * stride));
    switch(format_) {
    case TEXTURE_FORMAT_R_1UB_8:
        return Pixel(*src, 0, 0, 255);
    break;
    case TEXTURE_FORMAT_RGB_3UB_888:
        return Pixel(*src, *(src + 1), *(src + 2), 255);
    break;
    case TEXTURE_FORMAT_RGBA_4UB_8888:
        return Pixel(*src, *(src + 1), *(src + 2), *(src + 3));
    break;
    default:
        break;
    }

    /* Unsupported */
    S_WARN("Texture::pixel() is not yet implemented for format {0}", format_);
    return smlt::optional<Pixel>();
}

bool Texture::blur(BlurType blur_type, std::size_t radius) {
    if(blur_type != BLUR_TYPE_SIMPLE) {
        S_WARN("Unimplemented blur type: {0}", blur_type);
        return false;
    }

    if(radius == 0 || is_compressed() || is_paletted_format()) {
        S_WARN("Unsupported format or no radius when blurring");
        return false;
    }

    int blur_width = (radius * 2) + 1;
    std::vector<Pixel> bucket(blur_width * blur_width);

    std::vector<uint8_t> new_data(this->data_size());

    auto s = texture_format_stride(format_);

    for(int y = 0; y < height(); ++y) {
        for(int x = 0; x < width(); ++x) {
            int j0 = 0;
            int k0 = 0;

            float accum [4] = {0};

            // gather the colors from the surrouding box, including the
            // pixel we care about. If we go outside the bounds of the image
            // then we just assume a border (for now, probably there are better
            // options)
            for(int j = y - radius; j <= y + (int) radius; ++j, ++j0) {
                k0 = 0;
                for(int k = x - radius; k <= x + (int) radius; ++k, ++k0) {
                    int bucket_idx = (j0 * blur_width) + k0;
                    assert(bucket_idx < (int) bucket.size());

                    int ki = std::min(std::max(k, 0), width() - 1);
                    int ji = std::min(std::max(j, 0), height() - 1);
                    bucket[bucket_idx] = pixel(ki, ji).value_or(Pixel(255, 0, 255, 255));  // Default to a really glaring purple to show an error
                }
            }

            /* Go through the bucket and sum the rgba components */
            for(auto& pixel: bucket) {
                for(uint8_t i = 0; i < 4; ++i) {
                    accum[i] += pixel.rgba[i];
                }
            }

            /* Average into a new pixel */
            Pixel new_pixel;
            int i = 0;
            for(auto& v: accum) {
                v /= (blur_width * blur_width);
                new_pixel.rgba[i] = smlt::clamp(v, 0, 255);
                ++i;
            }

            /* Now convert that pixel into the target format */
            std::size_t target = (y * width() * s) + (x * s);
            uint8_t* dst = &new_data[target];
            auto final = new_pixel.to_format(format_);
            if(final.empty()) {
                S_ERROR("Unable to convert pixel to specified format");
                return false;
            }

            std::memcpy(dst, &final[0], final.size());
        }
    }

    set_data(new_data);

    return true;
}

void Texture::resize(uint16_t width, uint16_t height, uint32_t data_size) {
    if(width_ == width && height_ == height && data_size_ == data_size) {
        return;
    }

    width_ = width;
    height_ = height;
    resize_data(data_size);
}

void Texture::resize(uint16_t width, uint16_t height) {
    if(width_ == width && height_ == height) {
        return;
    }

    width_ = width;
    height_ = height;

    auto data_size = required_data_size(format_, width, height);
    resize_data(data_size);
}

static void explode_r8(const uint8_t* source, const TextureChannelSet& channels, float& r, float& g, float& b, float& a) {
    float sr = float(*source) / 255.0f;

    auto calculate_component = [&channels](uint8_t i, float sr, float sg, float sb, float sa) -> float {
        switch(channels[i]) {
            case TEXTURE_CHANNEL_ZERO: return 0.0f;
            case TEXTURE_CHANNEL_ONE: return 1.0f;
            case TEXTURE_CHANNEL_RED: return sr;
            case TEXTURE_CHANNEL_GREEN: return sg;
            case TEXTURE_CHANNEL_BLUE: return sb;
            case TEXTURE_CHANNEL_ALPHA: return sa;
            case TEXTURE_CHANNEL_INVERSE_RED: return 1.0f - sr;
        }

        return 0.0f;
    };

    r = calculate_component(0, sr, 0, 0, 0);
    g = calculate_component(1, sr, 0, 0, 0);
    b = calculate_component(2, sr, 0, 0, 0);
    a = calculate_component(3, sr, 0, 0, 0);
}

static void explode_rgb565(const uint8_t* source, const TextureChannelSet& channels, float& r, float& g, float& b, float& a) {
    uint16_t* texel = (uint16_t*) source;

    auto calculate_component = [&channels](uint8_t i, uint16_t texel) -> float {
        switch(channels[i]) {
            case TEXTURE_CHANNEL_ZERO: return 0.0f;
            case TEXTURE_CHANNEL_ONE: return 1.0f;
            case TEXTURE_CHANNEL_RED: return float((texel & 0xF800) >> 11) / 31.0f;
            case TEXTURE_CHANNEL_GREEN: return float((texel & 0x7E0) >> 5) / 63.0f;
            case TEXTURE_CHANNEL_BLUE: return float((texel & 0x1F)) / 31.0f;
            case TEXTURE_CHANNEL_ALPHA: return 1.0f;
            case TEXTURE_CHANNEL_INVERSE_RED: return 1.0f - (float((texel & 0xF800) >> 11) / 31.0f);
        }

        return 0.0f;
    };

    r = calculate_component(0, *texel);
    g = calculate_component(1, *texel);
    b = calculate_component(2, *texel);
    a = calculate_component(3, *texel);
}

static void compress_rgb565(uint8_t* dest, float r, float g, float b, float) {
    uint16_t* out = (uint16_t*) dest;

    uint8_t rr = (uint8_t) (31.0f * r);
    uint8_t rg = (uint8_t) (63.0f * g);
    uint8_t rb = (uint8_t) (31.0f * b);

    *out = (rr << 11) | (rg << 5) | (rb << 0);
}

static void compress_rgba4444(uint8_t* dest, float r, float g, float b, float a) {
    uint16_t* out = (uint16_t*) dest;

    uint8_t rr = (uint8_t) (15.0f * r);
    uint8_t rg = (uint8_t) (15.0f * g);
    uint8_t rb = (uint8_t) (15.0f * b);
    uint8_t ra = (uint8_t) (15.0f * a);

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

auto calculate_component = [](const TextureChannelSet& channels, uint8_t i, float sr, float sg, float sb, float sa) -> float {
    switch(channels[i]) {
        case TEXTURE_CHANNEL_ZERO: return 0.0f;
        case TEXTURE_CHANNEL_ONE: return 1.0f;
        case TEXTURE_CHANNEL_RED: return sr;
        case TEXTURE_CHANNEL_GREEN: return sg;
        case TEXTURE_CHANNEL_BLUE: return sb;
        case TEXTURE_CHANNEL_ALPHA: return sa;
        case TEXTURE_CHANNEL_INVERSE_RED: return 1.0f - sr;
    }

    return 0.0f;
};


static void explode_rgb888(const uint8_t* source, const TextureChannelSet& channels, float& r, float& g, float& b, float& a) {
    const float inv = 1.0f / 255.0f;

    float sr = float(source[0]) * inv;
    float sg = float(source[1]) * inv;
    float sb = float(source[2]) * inv;

    r = calculate_component(channels, 0, sr, sg, sb, 1.0f);
    g = calculate_component(channels, 1, sr, sg, sb, 1.0f);
    b = calculate_component(channels, 2, sr, sg, sb, 1.0f);
    a = 1.0f;
}

static void explode_rgba8888(const uint8_t* source, const TextureChannelSet& channels, float& r, float& g, float& b, float& a) {
    const float inv = 1.0f / 255.0f;

    float sr = float(source[0]) * inv;
    float sg = float(source[1]) * inv;
    float sb = float(source[2]) * inv;
    float sa = float(source[3]) * inv;

    r = calculate_component(channels, 0, sr, sg, sb, sa);
    g = calculate_component(channels, 1, sr, sg, sb, sa);
    b = calculate_component(channels, 2, sr, sg, sb, sa);
    a = calculate_component(channels, 3, sr, sg, sb, sa);
}

typedef void (*ExplodeFunc)(const uint8_t*, const TextureChannelSet&, float&, float&, float&, float&);
typedef void (*CompressFunc)(uint8_t*, float, float, float, float);

static const std::map<TextureFormat, ExplodeFunc> EXPLODERS = {
    {TEXTURE_FORMAT_R_1UB_8, explode_r8},
    {TEXTURE_FORMAT_RGB_1US_565, explode_rgb565},
    {TEXTURE_FORMAT_RGB_3UB_888, explode_rgb888},
    {TEXTURE_FORMAT_RGBA_4UB_8888, explode_rgba8888}
};

static const std::map<TextureFormat, CompressFunc> COMPRESSORS = {
    {TEXTURE_FORMAT_RGB_1US_565, compress_rgb565},
    {TEXTURE_FORMAT_RGBA_1US_4444, compress_rgba4444},
    {TEXTURE_FORMAT_RGBA_4UB_8888, compress_rgba8888}
};

bool Texture::convert(TextureFormat new_format, const TextureChannelSet &channels) {
    std::vector<uint8_t> original_data(data_size());
    std::copy(data_, data_ + data_size(), original_data.begin());

    auto original_format = format();

    set_format(new_format);

    auto failed = std::make_shared<bool>(false);

    mutate_data([=](uint8_t* data, uint16_t, uint16_t, TextureFormat nf) {
        _S_UNUSED(nf);
        assert(nf == new_format); // Make sure the new format was applied

        auto source_stride = texture_format_stride(original_format);
        auto dest_stride = texture_format_stride(new_format);

        const uint8_t* source_ptr = &original_data[0];
        uint8_t* dest_ptr = &data[0];

        if(!EXPLODERS.count(original_format) || !COMPRESSORS.count(new_format)) {
            S_ERROR("Unsupported texture conversion from {0} to {1}", original_format, new_format);
            *failed = true;
            return;
        }

        auto explode = EXPLODERS.at(original_format);
        auto compress = COMPRESSORS.at(new_format);

        for(auto i = 0u; i < original_data.size(); i += source_stride, source_ptr += source_stride, dest_ptr += dest_stride) {
            float r, g, b, a;

            explode(source_ptr, channels, r, g, b, a);
            compress(dest_ptr, r, g, b, a);
        }
    });

    if(*failed) {
        return false;
    } else {
        return true;
    }
}


static void do_flip_vertically(uint8_t* data, uint16_t width, uint16_t height, TextureFormat format) {
    /**
     *  Flips the texture data vertically
     */
    auto w = (uint32_t) width;
    auto h = (uint32_t) height;
    auto c = (uint32_t) texture_format_channels(format);

    auto row_size = w * c;

    uint8_t* src_row = &data[0];
    uint8_t* dst_row = &data[(h - 1) * row_size];
    std::vector<char> tmp(row_size);

    for(auto i = 0u; i < h / 2; ++i) {
        if(src_row != dst_row) {
            memcpy(&tmp[0], src_row, row_size);
            memcpy(src_row, dst_row, row_size);
            memcpy(dst_row, &tmp[0], row_size);
        }

        src_row += row_size;
        dst_row -= row_size;
    }
}

void Texture::flip_vertically() {
    mutate_data(&do_flip_vertically);
}

void Texture::set_source(const Path& source) {
    source_ = source;
}

void Texture::free() {
    /* We don't mark data dirty here, we don't want
     * anything to be updated in GL, we're just freeing
     * the RAM */

    delete [] data_;
    data_ = nullptr;
    data_size_ = 0;
}

bool Texture::has_data() const {
    return bool(data_);
}

void Texture::flush() {
    /* If in a coroutine: yield, run this code in the main thread, then resume */
    cr_run_main([this]() {
        renderer_->prepare_texture(this);
    });
}

void Texture::mutate_data(Texture::MutationFunc func) {
    func(&data_[0], width_, height_, format_);

    /* A mutation by definition updates the data */
    data_dirty_ = true;
}

bool Texture::is_compressed() const {
    switch(format_) {
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID:
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID:
    case TEXTURE_FORMAT_RGB_1US_565_VQ_TWID_MIP:
    case TEXTURE_FORMAT_ARGB_1US_4444_VQ_TWID_MIP:
    case TEXTURE_FORMAT_ARGB_1US_1555_VQ_TWID_MIP:
        return true;
    default:
        return false;
    }
}

uint8_t Texture::channels() const {
    return texture_format_channels(format_);
}

const uint8_t *Texture::data() const {
    return data_;
}

uint32_t Texture::data_size() const {
    return data_size_;
}

void Texture::set_data(const uint8_t* data, std::size_t size) {
    resize_data(size);
    std::copy(data, data + size, data_);
}

uint8_t* Texture::_stash_paletted_data() {
    if(paletted_data_) {
        delete [] paletted_data_;
    }

    paletted_data_ = new uint8_t[data_size()];
    std::copy(data_, data_ + data_size(), paletted_data_);
    return paletted_data_;
}

void Texture::save_to_file(const Path& filename) {
    _S_UNUSED(filename);
    assert(0 && "Not Implemented");
}

Path Texture::source() const {
    return source_;
}

TextureFilter Texture::texture_filter() const {
    return filter_;
}

TextureWrap Texture::wrap_u() const {
    return wrap_u_;
}

TextureWrap Texture::wrap_v() const {
    return wrap_v_;
}

TextureWrap Texture::wrap_w() const {
    return wrap_w_;
}

MipmapGenerate Texture::mipmap_generation() const {
    return mipmap_generation_;
}

void Texture::set_texture_filter(TextureFilter filter) {
    if(filter != filter_) {
        filter_ = filter;
        params_dirty_ = true;
    }
}

void Texture::set_free_data_mode(TextureFreeData mode) {
    if(free_data_mode_ != mode) {
        free_data_mode_ = mode;
        params_dirty_ = true;
    }
}

TextureFreeData Texture::free_data_mode() const {
    return free_data_mode_;
}

bool Texture::_params_dirty() const {
    return params_dirty_;
}

void Texture::_set_params_clean() {
    params_dirty_ = false;
}

bool Texture::_data_dirty() const {
    return data_dirty_;
}

void Texture::_set_data_clean() {
    data_dirty_ = false;
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

void Texture::set_auto_upload(bool v) {
    if(auto_upload_ != v) {
        auto_upload_ = v;
        params_dirty_ = true;
    }
}

void Texture::set_mipmap_generation(MipmapGenerate type) {
    if(mipmap_generation_ != type) {
        mipmap_generation_ = type;
        params_dirty_ = true;
    }
}

std::vector<uint8_t> Texture::data_copy() const {
    std::vector<uint8_t> result(data_size());
    std::copy(data_, data_ + data_size_, result.begin());
    return result;
}

void Texture::set_data(const std::vector<uint8_t> &d) {
    set_data(&d[0], d.size());
}

void Texture::_set_has_mipmaps(bool v) {
    has_mipmaps_ = v;
}

void Texture::resize_data(uint32_t byte_size) {
    if(byte_size == data_size_) {
        return;
    }

    // FIXME: Do we bother if byte_size is lower, and
    // close to data_size_? Optimisation to avoid realloc?

    if(data_) {
        delete [] data_;
        data_ = nullptr;
    }

    data_ = new uint8_t[byte_size];
    std::memset(data_, 0, byte_size);
    data_size_ = byte_size;
    data_dirty_ = true;
}

bool Texture::on_init() {
    // Tell the renderer about the texture
    S_DEBUG("Registering texture with the renderer: {0}", renderer_);
    renderer_->register_texture(id(), this);
    return true;
}

void Texture::on_clean_up() {
    // Tell the renderer to forget the texture
    renderer_->unregister_texture(id(), this);
}

bool Texture::has_mipmaps() const {
    return texture_format_contains_mipmaps(format_) || has_mipmaps_;
}

bool Texture::auto_upload() const {
    return auto_upload_;
}

void Texture::_set_renderer_specific_id(const uint32_t id) {
    renderer_id_ = id;
}

uint32_t Texture::_renderer_specific_id() const {
    return renderer_id_;
}

std::size_t texture_format_stride(TextureFormat format) {
    switch(format) {
        case TEXTURE_FORMAT_R_1UB_8: return 1;
        case TEXTURE_FORMAT_RGB_1US_565:
        case TEXTURE_FORMAT_RGB_1US_565_TWID:
        case TEXTURE_FORMAT_RGBA_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_4444:
        case TEXTURE_FORMAT_ARGB_1US_4444_TWID:
        case TEXTURE_FORMAT_ARGB_1US_1555:
        case TEXTURE_FORMAT_ARGB_1US_1555_TWID:
        case TEXTURE_FORMAT_RGBA_1US_5551: return 2;
        case TEXTURE_FORMAT_RGB_3UB_888: return 3;
        case TEXTURE_FORMAT_RGBA_4UB_8888: return 4;
    default:
        assert(0 && "Not implemented");
        return 0;
    }
}

std::vector<uint8_t> Pixel::to_format(TextureFormat fmt) {
    switch(fmt) {
    case TEXTURE_FORMAT_R_1UB_8:
        return {rgba[0]};
    case TEXTURE_FORMAT_RGB_3UB_888:
        return {rgba[0], rgba[1], rgba[2]};
    case TEXTURE_FORMAT_RGBA_4UB_8888:
        return {rgba[0], rgba[1], rgba[2], rgba[2]};
    default:
        S_WARN("Unsupported operation to convert pixel to format: {0}", fmt);
        return std::vector<uint8_t>();
    }
}

}
