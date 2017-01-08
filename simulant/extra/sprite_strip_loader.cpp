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

#include "sprite_strip_loader.h"

#include "../shortcuts.h"

namespace smlt {
namespace extra {

SpriteStripLoader::SpriteStripLoader(ResourceManager& rm, const std::string& filename, uint32_t frame_width):
    rm_(rm),
    filename_(filename),
    frame_width_(frame_width) {
}

std::vector<TextureID> SpriteStripLoader::load_frames() {
    //Load texture, but don't upload to OpenGL
    auto tmp = rm_.texture(rm_.new_texture_from_file(filename_));

    if(tmp->width() % frame_width_ != 0) {
        throw std::logic_error("Invalid texture width. Should be a multiple of: " + std::to_string(frame_width_));
    }

    std::vector< Texture::Data > frame_data;
    frame_data.resize(tmp->width() / frame_width_);

    /*
      Go through the loaded texture data in rows, keep track of the
      x position so we can copy the data to the correct frame
    */
    uint32_t bytes = tmp->bpp() / 8;
    for(uint32_t y = 0; y < tmp->height(); ++y) {
        uint32_t current_frame = 0;

        for(uint32_t x = 0; x < tmp->width(); ++x) {
            uint32_t idx = ((y * bytes) * tmp->width()) + (x * bytes);

            for(uint32_t i = 0; i < tmp->bpp() / 8; ++i) {
                frame_data[current_frame].push_back(tmp->data().at(idx + i));
            }

            if(x && (x % frame_width_) == 0) {
                current_frame++;
            }
        }
    }

    std::vector<TextureID> results;
    for(Texture::Data& data: frame_data) {
        auto tex = rm_.texture(rm_.new_texture());

        tex->set_bpp(tmp->bpp()); //Set the bpp
        tex->resize(frame_width_, tmp->height()); //Resize the texture
        tex->data().assign(data.begin(), data.end()); //Copy the frame data
        tex->upload(MIPMAP_GENERATE_COMPLETE, TEXTURE_WRAP_CLAMP_TO_EDGE, TEXTURE_FILTER_NEAREST, true);

        results.push_back(tex->id());
    }

    return results;
}

}
}
