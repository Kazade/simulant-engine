/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <cstdint>
#include <memory>
#include <queue>
#include <vector>
#include "generic/identifiable.h"
#include "generic/managed.h"
#include "generic/cow_vector.h"
#include "loadable.h"
#include "types.h"
#include "asset.h"
#include "interfaces.h"
#include "interfaces/updateable.h"

namespace smlt {

enum MipmapGenerate {
    MIPMAP_GENERATE_NONE,
    MIPMAP_GENERATE_COMPLETE
};

enum TextureWrap {
    TEXTURE_WRAP_REPEAT,
    TEXTURE_WRAP_CLAMP_TO_EDGE,
    TEXTURE_WRAP_MIRRORED_REPEAT,
    TEXTURE_WRAP_MIRRORED_CLAMP_TO_EDGE
};

enum TextureFilter {
    TEXTURE_FILTER_POINT,
    TEXTURE_FILTER_BILINEAR,
    TEXTURE_FILTER_TRILINEAR
};

/*
 * Simulant intentionally only supports a handful of formats for portability.
 *
 * This list isn't fixed though, if you need more, just file an issue.
 */
enum TextureFormat {
    // Standard formats
    TEXTURE_FORMAT_R8,
    TEXTURE_FORMAT_RGB888,
    TEXTURE_FORMAT_RGBA8888,

    // Packed short formats
    TEXTURE_FORMAT_RGBA4444,
    TEXTURE_FORMAT_RGBA5551,

    // Dreamcast PVR VQ compressed
    TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ,
    TEXTURE_FORMAT_UNSIGNED_SHORT_5_6_5_VQ_TWID,
    TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ,
    TEXTURE_FORMAT_UNSIGNED_SHORT_4_4_4_4_VQ_TWID,
    TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ,
    TEXTURE_FORMAT_UNSIGNED_SHORT_1_5_5_5_VQ_TWID,

    // S3TC
    TEXTURE_FORMAT_RGB_S3TC_DXT1_EXT,
    TEXTURE_FORMAT_RGBA_S3TC_DXT1_EXT,
    TEXTURE_FORMAT_RGBA_S3TC_DXT3_EXT,
    TEXTURE_FORMAT_RGBA_S3TC_DXT5_EXT,
};

uint8_t texture_format_stride(TextureFormat format);

enum TextureTexelType {
    TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE,
    TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_4_4_4_4,
    TEXTURE_TEXEL_TYPE_UNSIGNED_SHORT_5_5_5_1,
    TEXTURE_TEXEL_TYPE_UNSPECIFIED = 1000
};

TextureTexelType texel_type_from_texture_format(TextureFormat format);

enum TextureFreeData {
    TEXTURE_FREE_DATA_NEVER,
    TEXTURE_FREE_DATA_AFTER_UPLOAD
};

class Renderer;

enum TextureChannel {
    TEXTURE_CHANNEL_RED,
    TEXTURE_CHANNEL_GREEN,
    TEXTURE_CHANNEL_BLUE,
    TEXTURE_CHANNEL_ALPHA,
    TEXTURE_CHANNEL_ZERO,
    TEXTURE_CHANNEL_ONE
};

typedef std::array<TextureChannel, 4> TextureChannelSet;

class Texture :
    public Asset,
    public Loadable,
    public generic::Identifiable<TextureID>,
    public RefCounted<Texture>,
    public Updateable,
    public RenderTarget,
    public ChainNameable<Texture> {

public:
    static const TextureChannelSet DEFAULT_SOURCE_CHANNELS;

    struct BuiltIns {
        static const std::string CHECKERBOARD;
        static const std::string BUTTON;
    };

    typedef std::shared_ptr<Texture> ptr;
    typedef std::vector<uint8_t> Data;

    Texture(TextureID id, AssetManager* asset_manager, uint16_t width, uint16_t height, TextureFormat format=TEXTURE_FORMAT_RGBA8888);

    TextureFormat format() const;
    void set_format(TextureFormat format, TextureTexelType texel_type=TEXTURE_TEXEL_TYPE_UNSPECIFIED);

    /* Convert a texture to a new format and allow manipulating/filling the channels during the conversion */
    void convert(
        TextureFormat new_format,
        const TextureChannelSet& channels=Texture::DEFAULT_SOURCE_CHANNELS
    );

    /*
     * Change the width and height, but manually set the data buffer size,
     * mainly used for compressed textures
     */
    void resize(uint16_t width, uint16_t height, uint32_t data_size);

    /*
     * Change the width and height, automatically resizing the data buffer
     * depending on the bytes_per_pixel of the texel_type
     */
    void resize(uint16_t width, uint16_t height);

    /*
     * Flip the data buffer vertically. This will have no effect if the
     * data buffer has been wiped after upload
     */
    void flip_vertically();

    void set_source(const unicode& source);

    /* Texture filtering and wrapping */
    void set_texture_filter(TextureFilter filter);

    /* If set to TEXTURE_FREE_DATA_AFTER_UPLOAD then the data attribute will be
     * wiped after the renderer has uploaded to the GPU.
     */
    void set_free_data_mode(TextureFreeData mode);

    /* Set the texture wrap modes, either together or per-dimension */
    void set_texture_wrap(TextureWrap wrap_u, TextureWrap wrap_v, TextureWrap wrap_w);
    void set_texture_wrap_u(TextureWrap wrap_u);
    void set_texture_wrap_v(TextureWrap wrap_v);
    void set_texture_wrap_w(TextureWrap wrap_w);

    /* If enabled (default) the texture will be uploaded to the GPU
     * by the renderer. You can disable this if you need just a way
     * to load images from disk for other purposes (e.g. heightmaps)
     */
    void set_auto_upload(bool v=true);
    void set_mipmap_generation(MipmapGenerate type);

    const Texture::Data& data() const;

    void set_data(const uint8_t* data);
    void set_data(const Texture::Data& data);

    /* Clear the data buffer */
    void free();

    /* Returns true if the data array isn't empty */
    bool has_data() const;

    /*
     * Flushes texture data / properties to the renderer immediately. This
     * will free ram if the free data mode is set to TEXTURE_FREE_DATA_AFTER_UPLOAD
     */
    void flush();

    typedef std::function<void (uint8_t*, uint16_t, uint16_t, TextureFormat)> MutationFunc;

    /* Apply a mutation function to the current texture data */
    void mutate_data(MutationFunc func);

    TextureTexelType texel_type() const;
    uint16_t width() const override;
    uint16_t height() const override;
    Vec2 dimensions() const { return Vec2(width(), height()); }

    /*
     * Returns true if this Texture uses a compressed format
     */
    bool is_compressed() const;

    /* Returns the data size of each texel in bytes */
    std::size_t bytes_per_pixel() const;

    /* Returns the data size of each texel in bits */
    std::size_t bits_per_pixel() const;

    /*
     * Returns the number of channels that this texture has
     */
    uint8_t channels() const;

    /*
     * Save a texture to the specified file. Will only work for
     * uncompressed Textures
     */
    void save_to_file(const unicode& filename);

    unicode source() const;
    TextureFilter texture_filter() const;
    TextureWrap wrap_u() const;
    TextureWrap wrap_v() const;
    TextureWrap wrap_w() const;
    MipmapGenerate mipmap_generation() const;
    TextureFreeData free_data_mode() const;

    /* These are overridden to notify the renderer of texture changes */
    bool init() override;
    void clean_up() override;
    void update(float dt) override;
    bool has_mipmaps() const;
    bool auto_upload() const;


    /* This is for storing the GL (or whatever) texture ID */
    void _set_renderer_specific_id(const uint32_t id);
    uint32_t _renderer_specific_id() const;

    /* INTERNAL: Clears the params dirty flag */
    void _set_params_clean();

    /* INTERNAL: returns true if the data needs re-uploading */
    bool _data_dirty() const;

    /* INTERNAL: clears the dirty data flag */
    void _set_data_clean();

    /* INTERNAL: returns true if the filters are dirty */
    bool _params_dirty() const;
    void _set_has_mipmaps(bool v);
private:
    Renderer* renderer_ = nullptr;

    uint16_t width_ = 0;
    uint16_t height_ = 0;

    TextureTexelType texel_type_ = TEXTURE_TEXEL_TYPE_UNSIGNED_BYTE;
    TextureFormat format_ = TEXTURE_FORMAT_RGBA8888;

    unicode source_;

    bool auto_upload_ = true; /* If true, the texture is uploaded by the renderer asap */
    bool data_dirty_ = true;
    Texture::Data data_;
    TextureFreeData free_data_mode_ = TEXTURE_FREE_DATA_AFTER_UPLOAD;

    MipmapGenerate mipmap_generation_ = MIPMAP_GENERATE_COMPLETE;
    bool has_mipmaps_ = false;

    bool params_dirty_ = true;
    TextureFilter filter_ = TEXTURE_FILTER_POINT;
    TextureWrap wrap_u_ = TEXTURE_WRAP_REPEAT;
    TextureWrap wrap_v_ = TEXTURE_WRAP_REPEAT;
    TextureWrap wrap_w_ = TEXTURE_WRAP_REPEAT;

    uint32_t renderer_id_ = 0;
};

}

#endif // TEXTURE_H_INCLUDED
