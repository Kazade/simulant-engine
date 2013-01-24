#ifndef TEXTURE_H_INCLUDED
#define TEXTURE_H_INCLUDED

#include <cstdint>
#include <tr1/memory>
#include <vector>
#include "generic/identifiable.h"
#include "loadable.h"
#include "types.h"
#include "resource.h"

namespace kglt {

class Texture :
    public Resource,
    public Loadable,
    public generic::Identifiable<TextureID> {

public:
    typedef std::tr1::shared_ptr<Texture> ptr;
    typedef std::vector<uint8_t> Data;

    uint32_t gl_tex() const { return gl_tex_; }

    Texture(ResourceManager* resource_manager, TextureID id):
        Resource(resource_manager),
        generic::Identifiable<TextureID>(id),
        width_(0),
        height_(0),
        bpp_(32),
        gl_tex_(0) { }

    ~Texture();

    void set_bpp(uint32_t bits=32);
    void resize(uint32_t width, uint32_t height);
    void upload(bool free_after=false,
                bool generate_mipmaps=true,
                bool repeat=true,
                bool linear=false); //Upload to GL, initializes the tex ID
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
