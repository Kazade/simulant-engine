/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <cstdint>
#include <memory>
#include <vector>
#include "generic/identifiable.h"
#include "generic/managed.h"
#include "loadable.h"
#include "types.h"
#include "resource.h"
#include "interfaces.h"

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
    TEXTURE_FILTER_LINEAR,
    TEXTURE_FILTER_NEAREST
};

class Texture :
    public Resource,
    public Loadable,
    public generic::Identifiable<TextureID>,
    public Managed<Texture>,
    public RenderTarget {

public:
    struct BuiltIns {
        static const std::string CHECKERBOARD;
        static const std::string BUTTON;
    };

    typedef std::shared_ptr<Texture> ptr;
    typedef std::vector<uint8_t> Data;

    uint32_t gl_tex() const { return gl_tex_; }

    Texture(TextureID id, ResourceManager* resource_manager):
        Resource(resource_manager),
        generic::Identifiable<TextureID>(id),
        width_(0),
        height_(0),
        bpp_(32),
        gl_tex_(0) { }

    ~Texture();

    void set_bpp(uint32_t bits=32);
    void resize(uint32_t width, uint32_t height);
    void upload(
        MipmapGenerate mipmap = MIPMAP_GENERATE_COMPLETE,
        TextureWrap wrap = TEXTURE_WRAP_REPEAT,
        TextureFilter filter = TEXTURE_FILTER_NEAREST,
        bool free_after=true
    ); //Upload to GL, initializes the tex ID

    void __do_upload(MipmapGenerate mipmap, TextureWrap wrap, TextureFilter filter, bool free_after);

    void flip_vertically();
    void free(); //Frees the data used to construct the texture

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    uint32_t bpp() const { return bpp_; }
    uint8_t channels() const { return bpp_ / 8; }

    Texture::Data& data() { return data_; }

    void sub_texture(TextureID src, uint16_t offset_x, uint16_t offset_y);

private:
    uint32_t width_;
    uint32_t height_;
    uint32_t bpp_;

    Texture::Data data_;

    uint32_t gl_tex_;
};

}

#endif // TEXTURE_H_INCLUDED
