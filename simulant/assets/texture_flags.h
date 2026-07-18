#pragma once

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

enum TextureFreeData {
    TEXTURE_FREE_DATA_NEVER,
    TEXTURE_FREE_DATA_AFTER_UPLOAD
};

struct TextureFlags {
    TextureFlags(MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE,
                 TextureWrap wrap = TEXTURE_WRAP_REPEAT,
                 TextureFilter filter = TEXTURE_FILTER_POINT) :
        mipmap(mipmap), wrap(wrap), filter(filter) {}

    MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE;
    TextureWrap wrap = TEXTURE_WRAP_REPEAT;
    TextureFilter filter = TEXTURE_FILTER_POINT;
    TextureFreeData free_data = TEXTURE_FREE_DATA_AFTER_UPLOAD;
    bool flip_vertically = false;
    bool auto_upload = true; // Should the texture be uploaded automatically?

    /* If true (the default), loading a texture with the same resolved
     * source path as an already-loaded texture in the same AssetManager
     * will return the existing texture instead of loading it again. */
    bool use_asset_cache = true;
};
} // namespace smlt
